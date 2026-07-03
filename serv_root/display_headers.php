#!/usr/bin/php
<?php
// CGI scripts require headers first, followed by a blank line
echo "Status: 200 OK\r\n";
echo "Content-Type: text/html; charset=UTF-8\r\n";
echo "\r\n";
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI Environment Variables</title>
    <style>
        body { font-family: sans-serif; background: #f4f4f9; padding: 20px; color: #333; }
        h1 { color: #2c3e50; border-bottom: 2px solid #2c3e50; padding-bottom: 10px; }
        table { width: 100%; border-collapse: collapse; margin-top: 20px; background: white; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background-color: #2c3e50; color: white; }
        tr:hover { background-color: #f1f1f1; }
        .http-var { color: #e74c3c; font-weight: bold; }
        .cgi-var { color: #2980b9; font-weight: bold; }
    </style>
</head>
<body>

    <h1>CGI Environment Variables</h1>
    <p>This page displays all environment variables received by the CGI script.</p>

    <table>
        <thead>
            <tr>
                <th>Variable Name</th>
                <th>Value</th>
            </tr>
        </thead>
        <tbody>
            <?php
            // Get all environment variables passed by your C++ server
            $env = $_SERVER;
            
            // Sort them alphabetically by key
            ksort($env);

            foreach ($env as $key => $value) {
                // Highlight HTTP_ headers differently for easy checking
                $class = (strpos($key, 'HTTP_') === 0) ? 'http-var' : 'cgi-var';
                
                echo "<tr>";
                echo "<td class='{$class}'>" . htmlspecialchars($key) . "</td>";
                echo "<td>" . htmlspecialchars($value) . "</td>";
                echo "</tr>";
            }
            ?>
        </tbody>
    </table>

</body>
</html>
