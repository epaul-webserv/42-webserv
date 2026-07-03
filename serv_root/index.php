<?php
/**
 * PHP CGI Environment Inspector & Cookie Diagnostic Dashboard
 * 100% self-contained. No external dependencies.
 */

// 1. Core Cookie Configuration
$cookie_name = 'cgi_test_cookie';
$cookie_found = isset($_COOKIE[$cookie_name]);
$cookie_value = '';

if (!$cookie_found) {
    // Inject a token payload if not present
    $cookie_value = 'PHP-CGI-Working-' . date('H:i:s');
    setcookie($cookie_name, $cookie_value, time() + 3600, "/");
} else {
    $cookie_value = $_COOKIE[$cookie_name];
}

// 2. Handle incoming POST tests
$post_received = !empty($_POST);
$test_input = $_POST['test_input'] ?? '';

// 3. Catch headers safely across odd CGI gateways
$headers = function_exists('getallheaders') ? getallheaders() : [];
if (empty($headers)) {
    foreach ($_SERVER as $name => $value) {
        if (substr($name, 0, 5) == 'HTTP_') {
            $headers[str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($name, 5)))))] = $value;
        }
    }
}

// 4. Critical Gateway Health Metrics
$cgi_checks = [
    'Gateway Interface' => $_SERVER['GATEWAY_INTERFACE'] ?? 'Direct Process Execution',
    'Server Software' => $_SERVER['SERVER_SOFTWARE'] ?? 'Unknown',
    'PHP Interface Mode' => php_sapi_name(),
    'HTTPS Protocol' => (!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off') ? '🔒 Secure' : '🔓 Unsecure / Plain HTTP',
];
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Integrated CGI Gateway & Cookie Inspector</title>
    <style>
        :root {
            --bg: #090d16;
            --surface: #131926;
            --surface-hover: #1e2638;
            --border: #26334d;
            --text-main: #f1f5f9;
            --text-muted: #94a3b8;
            --accent: #38bdf8;
            --accent-purple: #a78bfa;
            --code-bg: #0b0f19;
            
            /* Dynamic status variables mapped from PHP */
            --status-color: <?php echo $cookie_found ? '#34d399' : '#38bdf8'; ?>;
            --status-bg: <?php echo $cookie_found ? 'rgba(52, 211, 153, 0.1)' : 'rgba(56, 189, 248, 0.1)'; ?>;
            --status-border: <?php echo $cookie_found ? '#34d399' : '#38bdf8'; ?>;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background-color: var(--bg);
            color: var(--text-main);
            margin: 0;
            padding: 40px 20px;
            display: flex;
            justify-content: center;
        }

        .wrapper { width: 100%; max-width: 1000px; }

        header {
            margin-bottom: 32px;
            border-bottom: 1px solid var(--border);
            padding-bottom: 24px;
            display: flex;
            justify-content: space-between;
            align-items: flex-end;
            flex-wrap: wrap;
            gap: 16px;
        }

        h1 {
            font-size: 28px;
            font-weight: 800;
            margin: 0 0 6px 0;
            letter-spacing: -0.02em;
            background: linear-gradient(to right, #38bdf8, #a78bfa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        header p { margin: 0; color: var(--text-muted); font-size: 14px; }

        .badge-sapi {
            background-color: var(--border);
            border: 1px solid var(--accent);
            color: var(--accent);
            padding: 4px 12px;
            border-radius: 6px;
            font-family: monospace;
            font-size: 13px;
        }

        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
            gap: 24px;
            margin-bottom: 24px;
        }

        .card {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 12px;
            padding: 24px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.25);
        }

        .card h2 {
            font-size: 16px;
            font-weight: 600;
            margin: 0 0 16px 0;
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .cookie-status-alert {
            background: var(--status-bg);
            border: 1px solid var(--status-border);
            color: var(--status-color);
            padding: 14px;
            border-radius: 8px;
            font-size: 13.5px;
            line-height: 1.5;
            margin-bottom: 16px;
        }

        .kv-table { width: 100%; border-collapse: collapse; font-size: 13.5px; }
        .kv-table td { padding: 10px 0; border-bottom: 1px solid rgba(38, 51, 77, 0.5); vertical-align: top; }
        .kv-table tr:last-child td { border-bottom: none; }
        
        .key { color: var(--text-muted); font-weight: 500; width: 40%; word-break: break-all; padding-right: 8px; }
        .value { font-family: monospace; color: var(--text-main); word-break: break-all; text-align: right; }
        .value-highlight { color: #34d399; }

        .dump-box {
            background: var(--code-bg);
            border: 1px solid var(--border);
            border-radius: 8px;
            padding: 14px;
            font-family: monospace;
            font-size: 12.5px;
            overflow-x: auto;
            max-height: 200px;
            margin: 0;
        }

        .form-group { display: flex; gap: 12px; margin-top: 12px; }
        
        input[type="text"] {
            flex: 1; background: var(--code-bg); border: 1px solid var(--border);
            border-radius: 6px; padding: 10px 14px; color: var(--text-main); font-size: 14px; outline: none;
        }
        input[type="text"]:focus { border-color: var(--accent); }

        .btn {
            display: inline-flex; justify-content: center; align-items: center;
            padding: 10px 18px; border-radius: 6px; font-size: 13.5px; font-weight: 600;
            text-decoration: none; cursor: pointer; border: none; transition: opacity 0.15s;
        }
        .btn:hover { opacity: 0.9; }
        .btn-primary { background: var(--accent); color: var(--bg); }
        .btn-secondary { background: rgba(255,255,255,0.05); color: var(--text-main); border: 1px solid var(--border); }
        .btn-danger { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid #ef4444; }

        .action-bar { display: flex; gap: 12px; margin-top: 16px; }
    </style>
</head>
<body>

<div class="wrapper">
    <header>
        <div>
            <h1>Gateway Inspector Pro</h1>
            <p>Unified Cookie State, Request Pipeline, & SAPI Tracker</p>
        </div>
        <div class="badge-sapi">Engine: <?php echo php_sapi_name(); ?></div>
    </header>

    <!-- Row 1: Cookies & Core Gateway Stats -->
    <div class="grid">
        <!-- Cookie Tracker Component -->
        <div class="card">
            <h2>🍪 Live Cookie Diagnostics</h2>
            
            <div class="cookie-status-alert">
                <?php if ($cookie_found): ?>
                    <strong>Active State Detected:</strong> CGI environmental variable passthrough is working perfectly! Cookies are holding across requests.
                <?php else: ?>
                    <strong>Awaiting Verification:</strong> An initial cookie value has been injected into your browser. Click <strong>Refresh</strong> to see if it persists.
                <?php endif; ?>
            </div>

            <table class="kv-table">
                <tr>
                    <td class="key">Tracking Variable</td>
                    <td class="value" style="color: var(--accent-purple);"><?php echo htmlspecialchars($cookie_name); ?></td>
                </tr>
                <tr>
                    <td class="key">Payload Context</td>
                    <td class="value"><?php echo htmlspecialchars($cookie_value); ?></td>
                </tr>
            </table>

            <div class="action-bar">
                <a href="?" class="btn btn-primary" style="flex: 1;">🔄 Refresh & Verify</a>
                <?php if ($cookie_found): ?>
                    <button onclick="clearCookieAndReload()" class="btn btn-danger">🗑️ Nuke Cookie</button>
                <?php endif; ?>
            </div>
        </div>

        <!-- System Settings -->
        <div class="card">
            <h2>⚙️ Core Gateway Profile</h2>
            <table class="kv-table">
                <?php foreach ($cgi_checks as $k => $v): ?>
                <tr>
                    <td class="key"><?php echo htmlspecialchars($k); ?></td>
                    <td class="value value-highlight"><?php echo htmlspecialchars($v); ?></td>
                </tr>
                <?php endforeach; ?>
                <tr>
                    <td class="key">Active Query Errors</td>
                    <td class="value" style="color: <?php echo isset($_GET['?yolo']) ? '#f87171' : '#34d399'; ?>;">
                        <?php echo isset($_GET['?yolo']) ? '⚠️ URL includes literal "?" in key!' : 'None Detected'; ?>
                    </td>
                </tr>
            </table>
        </div>
    </div>

    <!-- Row 2: Live Sandbox Variables -->
    <div class="grid">
        <div class="card">
            <h2>🧪 POST Request Payload Sandbox</h2>
            <p style="color: var(--text-muted); font-size: 13px; margin: 0 0 12px 0;">Test how your gateway environment captures and holds form payloads.</p>
            <form method="POST" action="?">
                <div class="form-group">
                    <input type="text" name="test_input" placeholder="Type data payload string..." value="<?php echo htmlspecialchars($test_input); ?>" required>
                    <button type="submit" class="btn btn-secondary">Send POST</button>
                </div>
            </form>
            <?php if ($post_received): ?>
                <div style="color: #34d399; font-size: 12px; margin-top: 10px; font-weight: 500;">✓ Form payload safely processed by web gateway container.</div>
            <?php endif; ?>
        </div>

        <div class="card">
            <h2>🍪 Raw $_COOKIE Array</h2>
            <pre class="dump-box" style="color: #a78bfa;"><?php echo empty($_COOKIE) ? '// No parsed cookies present in this context.' : htmlspecialchars(print_r($_COOKIE, true)); ?></pre>
        </div>
    </div>

    <!-- Row 3: Scoped Memory Dumps -->
    <div class="grid">
        <div class="card">
            <h2>📥 URL Arguments ($_GET)</h2>
            <pre class="dump-box" style="color: #67e8f9;"><?php echo empty($_GET) ? '// Array empty. Append query string arguments to test.' : htmlspecialchars(print_r($_GET, true)); ?></pre>
        </div>
        <div class="card">
            <h2>📤 Active Form Data ($_POST)</h2>
            <pre class="dump-box" style="color: #fca5a5;"><?php echo empty($_POST) ? '// Array empty. Use the form payload box above.' : htmlspecialchars(print_r($_POST, true)); ?></pre>
        </div>
    </div>

    <!-- Incoming Network Request Headers -->
    <div class="card" style="margin-bottom: 24px;">
        <h2>🌐 Captured HTTP Request Headers</h2>
        <table class="kv-table">
            <?php foreach ($headers as $key => $val): ?>
            <tr>
                <td class="key" style="font-family: monospace; color: #f472b6;"><?php echo htmlspecialchars($key); ?></td>
                <td class="value"><?php echo htmlspecialchars($val); ?></td>
            </tr>
            <?php endforeach; ?>
        </table>
    </div>

    <!-- Full Diagnostics Configuration -->
    <div class="card">
        <h2>📂 Full Engine Scope Environment ($_SERVER)</h2>
        <pre class="dump-box" style="max-height: 350px; color: #cbd5e1;"><?php echo htmlspecialchars(print_r($_SERVER, true)); ?></pre>
    </div>
</div>

<script>
function clearCookieAndReload() {
    // JavaScript Client-side drop mechanism 
    document.cookie = "<?php echo $cookie_name; ?>=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
    // Force immediate redirection stripping query variables
    window.location.href = window.location.pathname;
}
</script>

</body>
</html>
