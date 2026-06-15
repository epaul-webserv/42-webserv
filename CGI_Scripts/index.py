#!/usr/bin/env python3
import cgitb

# Enable browser troubleshooting (shows Python errors if something breaks)
cgitb.enable()

# STEP 1: Print the HTTP header. 
# The blank line (\n) at the end is mandatory to separate headers from content!
print("Content-Type: text/html\n")

# STEP 2: Print the HTML content
print("""
<!DOCTYPE html>
<html>
<head>
    <title>CGI Hello World</title>
</head>
<body>
    <h1>Hello, World!</h1>
    <p>This page was generated dynamically using a Python CGI script.</p>
</body>
</html>
""")