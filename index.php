
<?php
// 1. Send the mandatory HTTP header followed by a blank line
header("Content-Type: text/html; charset=UTF-8");
echo "\n"; 

// 2. Output your content
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Hello World CGI</title>
</head>
<body>
    <h1><?php echo "Hello, World!"?></h1>
    <p>Served to you via PHP CGI.</p>
</body>
</html>