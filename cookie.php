<?php
// 1. Core Config
$cookie_name = 'cgi_test_cookie';

// 2. PHP-side Cookie Checking
$cookie_found = isset($_COOKIE[$cookie_name]);
$cookie_value = '';

if (!$cookie_found) {
    // Attempt to set a new cookie expiring in 1 hour
    $cookie_value = 'PHP-CGI-Working-' . date('H:i:s');
    setcookie($cookie_name, $cookie_value, time() + 3600, "/");
} else {
    $cookie_value = $_COOKIE[$cookie_name];
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PHP CGI Cookie Diagnostics</title>
    <style>
        :root {
            --bg-color: #f8fafc;
            --card-bg: #ffffff;
            --text-main: #0f172a;
            --text-muted: #64748b;
            --border-color: #e2e8f0;
            --status-color: <?php echo $cookie_found ? '#10b981' : '#3b82f6'; ?>;
            --status-bg: <?php echo $cookie_found ? '#ecfdf5' : '#eff6ff'; ?>;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-main);
            margin: 0;
            padding: 40px 20px;
            display: flex;
            justify-content: center;
        }

        .container { width: 100%; max-width: 600px; }
        .header { margin-bottom: 24px; }
        .header h1 { font-size: 24px; font-weight: 700; margin: 0 0 8px 0; color: #1e293b; }
        .header p { margin: 0; color: var(--text-muted); font-size: 14px; }
        
        .card {
            background: var(--card-bg);
            border: 1px solid var(--border-color);
            border-radius: 12px;
            padding: 24px;
            box-shadow: 0 1px 3px 0 rgba(0, 0, 0, 0.05);
            margin-bottom: 20px;
        }

        .status-badge {
            display: inline-flex;
            align-items: center;
            padding: 6px 12px;
            background-color: var(--status-bg);
            color: var(--status-color);
            border-radius: 9999px;
            font-size: 13px;
            font-weight: 600;
            margin-bottom: 20px;
        }

        .status-badge::before {
            content: '';
            display: inline-block;
            width: 8px;
            height: 8px;
            background-color: var(--status-color);
            border-radius: 50%;
            margin-right: 8px;
        }

        .data-row {
            display: flex;
            justify-content: space-between;
            padding: 12px 0;
            border-bottom: 1px solid var(--border-color);
            font-size: 14px;
        }

        .data-row:last-of-type { border-bottom: none; }
        .data-label { color: var(--text-muted); font-weight: 500; }
        
        .data-value {
            font-family: monospace;
            background: #f1f5f9;
            padding: 2px 6px;
            border-radius: 4px;
            font-size: 13px;
            max-width: 60%;
            word-break: break-all;
            text-align: right;
        }

        .button-group { display: flex; gap: 10px; margin-top: 8px; }
        
        .btn {
            flex: 1;
            display: inline-flex;
            justify-content: center;
            align-items: center;
            padding: 10px 16px;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
            text-decoration: none;
            cursor: pointer;
            border: 1px solid transparent;
        }

        .btn-primary { background-color: #0f172a; color: #ffffff; }
        .btn-primary:hover { background-color: #1e293b; }
        .btn-secondary { background-color: #ffffff; color: #475569; border-color: var(--border-color); }
        .btn-secondary:hover { background-color: #f8fafc; }

        .debug-dump {
            background: #0f172a;
            color: #38bdf8;
            padding: 16px;
            border-radius: 8px;
            font-family: monospace;
            font-size: 12px;
            overflow-x: auto;
            margin-top: 8px;
        }

        .debug-title { font-size: 12px; text-transform: uppercase; color: var(--text-muted); margin-bottom: 8px; font-weight: 600; }
    </style>
</head>
<body>

<div class="container">
    <div class="header">
        <h1>CGI Cookie Diagnostics</h1>
        <p>Testing environment variables, header pass-through, and array state.</p>
    </div>

    <div class="card">
        <?php if ($cookie_found): ?>
            <div class="status-badge">Active Cookie Detected</div>
            <p style="margin: 0 0 16px 0; font-size: 14px; color: var(--text-muted);">
                Your CGI setup is successfully persisting environment headers across requests!
            </p>
        <?php else: ?>
            <div class="status-badge">Initial State Set</div>
            <p style="margin: 0 0 16px 0; font-size: 14px; color: var(--text-muted);">
                No active session cookie found. A fresh token payload was injected. <strong>Refresh the page</strong> to see if it sticks.
            </p>
        <?php endif; ?>

        <div class="data-row">
            <span class="data-label">Target Parameter</span>
            <span class="data-value"><?php echo htmlspecialchars($cookie_name); ?></span>
        </div>
        <div class="data-row">
            <span class="data-label">Payload Value</span>
            <span class="data-value"><?php echo htmlspecialchars($cookie_value); ?></span>
        </div>
    </div>

    <div class="button-group">
        <a href="?" class="btn btn-primary">🔄 Refresh & Verify</a>
        <?php if ($cookie_found): ?>
            <button onclick="clearCookieAndReload()" class="btn btn-secondary">🗑️ Clear Cookie</button>
        <?php endif; ?>
    </div>

    <div style="margin-top: 32px;">
        <div class="debug-title">Global $_COOKIE Array Output</div>
        <pre class="debug-dump"><?php 
            if (empty($_COOKIE)) {
                echo "// Array is currently empty";
            } else {
                echo htmlspecialchars(print_r($_COOKIE, true)); 
            }
        ?></pre>
    </div>
</div>

<script>
function clearCookieAndReload() {
    // Force expire the cookie directly in the browser via JavaScript
    document.cookie = "<?php echo $cookie_name; ?>=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
    
    // Hard refresh stripping any query strings
    window.location.href = window.location.pathname;
}
</script>

</body>
</html>
