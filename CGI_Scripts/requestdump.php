#!/usr/bin/php
<?php
// Ensure plain text or HTML output (HTML used here for better readability)
header("Content-Type: text/html; charset=UTF-8");
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI / PHP Request Tester</title>
    <style>
        body { font-family: sans-serif; margin: 20px; line-height: 1.5; background: #f9f9f9; color: #333; }
        h1 { color: #0056b3; border-bottom: 2px solid #0056b3; padding-bottom: 10px; }
        h2 { color: #555; margin-top: 30px; }
        pre { background: #272822; color: #f8f8f2; padding: 15px; border-radius: 5px; overflow-x: auto; font-size: 14px; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
        @media (max-width: 768px) { .grid { grid-template-columns: 1fr; } }
    </style>
</head>
<body>

    <h1>CGI Request Details Tester</h1>
    <p><strong>Timestamp:</strong> <?php echo date('Y-m-d H:i:s T'); ?></p>
    <p><strong>PHP Version:</strong> <?php echo phpversion(); ?> (<?php echo php_sapi_name(); ?> SAPI)</p>

    <div class="grid">
        <div>
            <h2>--- Request Method & Basic Info ---</h2>
            <pre>
<strong>Method:</strong> <?php echo $_SERVER['REQUEST_METHOD'] ?? 'N/A'; ?>
<br><strong>Protocol:</strong> <?php echo $_SERVER['SERVER_PROTOCOL'] ?? 'N/A'; ?>
<br><strong>Request URI:</strong> <?php echo $_SERVER['REQUEST_URI'] ?? 'N/A'; ?>
<br><strong>Query String:</strong> <?php echo $_SERVER['QUERY_STRING'] ?? '(empty)'; ?>
            </pre>
        </div>

        <div>
            <h2>--- HTTP Request Headers ---</h2>
            <pre><?php
                $headers = function_exists('getallheaders') ? getallheaders() : [];
                if (empty($headers)) {
                    // Fallback if getallheaders() isn't available in CGI mode
                    foreach ($_SERVER as $name => $value) {
                        if (substr($name, 0, 5) == 'HTTP_') {
                            $headers[str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($name, 5)))))] = $value;
                        }
                    }
                }
                echo htmlspecialchars(print_r($headers, true));
            ?></pre>
        </div>
    </div>

    <h2>--- GET Data ($_GET) ---</h2>
    <pre><?php echo htmlspecialchars(print_r($_GET, true)); ?></pre>

    <h2>--- POST Data ($_POST) ---</h2>
    <pre><?php echo htmlspecialchars(print_r($_POST, true)); ?></pre>

    <h2>--- Raw Input Body (php://input) ---</h2>
    <pre><?php 
        $raw_input = file_get_contents('php://input');
        echo htmlspecialchars($raw_input ? $raw_input : '(No raw body data)'); 
    ?></pre>

    <h2>--- Full Environment / Server Variables ($_SERVER) ---</h2>
    <pre><?php echo htmlspecialchars(print_r($_SERVER, true)); ?></pre>

</body>
</html>