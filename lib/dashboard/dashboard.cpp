#include <pgmspace.h>
#include "dashboard.h"

const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Doorlock Dashboard</title>
  <style>
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background-color: #f5f6fa;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    h1 {
      margin-top: 20px;
      color: #2c3e50;
    }

    .container {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
      gap: 20px;
      width: 95%;
      max-width: 1000px;
      margin: 20px auto;
    }

    .panel {
      background: #fff;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      padding: 20px;
    }

    .panel h2 {
      font-size: 20px;
      margin-bottom: 10px;
      color: #34495e;
    }

    #camera-feed {
      width: 100%;
      height: 240px;
      background: #dcdde1;
      display: flex;
      align-items: center;
      justify-content: center;
      color: #888;
      border-radius: 8px;
    }

    form {
      display: flex;
      flex-direction: column;
      gap: 10px;
    }

    input[type="text"] {
      padding: 10px;
      border: 1px solid #ccc;
      border-radius: 6px;
      font-size: 16px;
    }

    button {
      padding: 10px 20px;
      background-color: #2980b9;
      color: #fff;
      border: none;
      border-radius: 6px;
      cursor: pointer;
      transition: background 0.3s;
      font-size: 16px;
    }

    button:hover {
      background-color: #3498db;
    }

    #register-output {
      margin-top: 10px;
      font-size: 14px;
      padding: 8px;
      border-radius: 6px;
      background-color: #ecf0f1;
      color: #2c3e50;
      min-height: 24px;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 10px;
    }

    th, td {
      text-align: left;
      padding: 10px;
      border-bottom: 1px solid #eee;
    }

    th {
      background: #f0f0f0;
      color: #333;
    }

    .remove-btn {
      background-color: #e74c3c;
      color: white;
      border: none;
      border-radius: 6px;
      padding: 5px 10px;
      cursor: pointer;
    }

    .remove-btn:hover {
      background-color: #c0392b;
    }

    @media (max-width: 768px) {
      .container {
        grid-template-columns: 1fr;
      }
    }
  </style>
</head>
<body>
  <h1>Smart Doorlock Dashboard</h1>

  <div class="container">
    <!-- Camera Feed Panel -->
    <div class="panel">
      <h2>Live Camera Feed</h2>
      <div id="camera-feed">
        <p>Camera feed placeholder</p>
      </div>
    </div>

    <!-- Registration Panel -->
    <div class="panel">
      <h2>Register New User</h2>
      <form id="register-form">
        <input type="text" id="username" placeholder="Enter user name" required />
        <button type="submit">Register User</button>
      </form>
      <div id="register-output">Awaiting action...</div>
    </div>

    <!-- Registered Users List -->
    <div class="panel">
      <h2>Registered Users</h2>
      <table id="users-table">
        <thead>
          <tr>
            <th>Name</th>
            <th>ID</th>
            <th>Last Seen</th>
            <th>Action</th>
          </tr>
        </thead>
        <tbody id="users-body">
        </tbody>
      </table>
    </div>
  </div>

  <script>
    const baseURL = window.location.origin; // e.g. http://doorlock.local

    // Handle registration form
    document.getElementById("register-form").addEventListener("submit", async (e) => {
      e.preventDefault();
      const name = document.getElementById("username").value;
      const output = document.getElementById("register-output");
      output.innerText = "Registering user...";
      try {
        // Send register command with name param
        const res = await fetch(`${baseURL}/register?name=${encodeURIComponent(name)}`);
        const data = await res.json();
      if(data.status == "ok"){
        output.style.backgroundColor = "#d4edda";
        output.innerText = `Success: ${name} has been registered`;
        // Refresh the users list
        addUserToList(name, data.user_id, new Date().toLocaleString());
      }
        else{
        output.style.backgroundColor = "#f8d7da";
        output.innerText = "Error: " + data.error;}
      } 
        catch (err) {
        output.style.backgroundColor = "#f8d7da";
        output.innerText = "Error: " + err.message;
      }
    });

    // Mock user list manipulation
    function addUserToList(name, id, time) {
      const tbody = document.getElementById("users-body");
      const tr = document.createElement("tr");
      tr.innerHTML = `
        <td>${name}</td>
        <td>${id}</td>
        <td>${time}</td>
        <td><button class="remove-btn" onclick="removeUser(${id})">-</button></td>
      `;
      tbody.appendChild(tr);
    }

    async function removeUser(id) {
      const res = await fetch(`${baseURL}/delete?id=${encodeURIComponent(id)}`);
      const tbody = document.getElementById("users-body");
      const rows = tbody.querySelectorAll("tr");
      rows.forEach(row => {
        if (row.children[1].innerText == id) {
          tbody.removeChild(row);
        }
      });
      // Later: send delete request to ESP
      console.log("Remove user ID:", id);
    }
  </script>
</body>
</html>
)rawliteral";
