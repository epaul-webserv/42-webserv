#!/usr/bin/env python3
import os
import sys
import urllib.parse
import http.cookies

def main():
    # 1. Print HTTP Headers required by CGI spec
    print("Content-Type: text/html; charset=utf-8")
    
    # Check if the user is submitting a new cookie via the form
    form_cookie_header = handle_cookie_submission()
    if form_cookie_header:
        print(form_cookie_header)
        
    print()  # End of headers

    # 2. Read Request Body (if any) safely based on CONTENT_LENGTH
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    except ValueError:
        content_length = 0

    body_data = sys.stdin.read(content_length) if content_length > 0 else ""

    # 3. Parse Raw Cookies
    raw_cookie_str = os.environ.get('HTTP_COOKIE', '')
    cookie_jar = http.cookies.SimpleCookie()
    if raw_cookie_str:
        cookie_jar.load(raw_cookie_str)

    # 4. Generate the HTML Output
    html_template = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI Tester Engine</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background-color: #f4f6f9;
            color: #333;
            margin: 0;
            padding: 40px 20px;
        }}
        .container {{
            max-width: 900px;
            margin: 0 auto;
        }}
        h1 {{
            color: #1e293b;
            border-bottom: 2px solid #e2e8f0;
            padding-bottom: 10px;
            margin-bottom: 30px;
        }}
        .card {{
            background: #ffffff;
            border-radius: 8px;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
            padding: 24px;
            margin-bottom: 25px;
        }}
        .card-title {{
            font-size: 1.2rem;
            font-weight: 600;
            color: #0f172a;
            margin-top: 0;
            margin-bottom: 15px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }}
        .badge {{
            font-size: 0.75rem;
            padding: 4px 8px;
            border-radius: 4px;
            font-weight: bold;
            background: #e2e8f0;
        }}
        .badge.method-get {{ background: #dcfce7; color: #15803d; }}
        .badge.method-post {{ background: #dbeafe; color: #1d4ed8; }}
        
        .interactive-grid {{
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }}
        @media (max-width: 768px) {{
            .interactive-grid {{ grid-template-columns: 1fr; }}
        }}
        
        table {{
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
        }}
        th, td {{
            text-align: left;
            padding: 10px 12px;
            border-bottom: 1px solid #f1f5f9;
            font-size: 0.95rem;
        }}
        th {{
            background-color: #f8fafc;
            color: #64748b;
            font-weight: 600;
            width: 30%;
        }}
        td {{
            font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, monospace;
            color: #0f172a;
            word-break: break-all;
        }}
        .env-important {{
            background-color: #fef08a !important;
            font-weight: bold;
        }}
        pre {{
            background: #0f172a;
            color: #f8fafc;
            padding: 15px;
            border-radius: 6px;
            overflow-x: auto;
            font-family: "SFMono-Regular", Consolas, monospace;
            margin: 0;
        }}
        .form-group {{
            margin-bottom: 15px;
        }}
        label {{
            display: block;
            margin-bottom: 5px;
            font-weight: 500;
            font-size: 0.9rem;
        }}
        input[type="text"], textarea {{
            width: 100%;
            padding: 8px 12px;
            border: 1px solid #cbd5e1;
            border-radius: 4px;
            box-sizing: border-box;
        }}
        textarea {{
            resize: vertical;
            font-family: inherit;
        }}
        button {{
            background: #2563eb;
            color: white;
            border: none;
            padding: 9px 16px;
            border-radius: 4px;
            cursor: pointer;
            font-weight: 500;
        }}
        button:hover {{ background: #1d4ed8; }}
        button.btn-post {{ background: #dc2626; }}
        button.btn-post:hover {{ background: #b91c1c; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>CGI Execution Context Diagnostics</h1>

        <!-- Request Summary Card -->
        <div class="card">
            <div class="card-title">
                Request Summary
                <span class="badge method-{os.environ.get('REQUEST_METHOD', 'GET').lower()}">
                    {os.environ.get('REQUEST_METHOD', 'GET')}
                </span>
            </div>
            <table>
                <tr><th>Protocol</th><td>{os.environ.get('SERVER_PROTOCOL', 'N/A')}</td></tr>
                <tr><th>URI Requested</th><td>{os.environ.get('REQUEST_URI', os.environ.get('SCRIPT_NAME', 'N/A'))}</td></tr>
                <tr><th>Query String</th><td>{os.environ.get('QUERY_STRING', '(None)')}</td></tr>
                <tr><th>Path Info</th><td>{os.environ.get('PATH_INFO', '(None)')}</td></tr>
            </table>
        </div>

        <!-- Forms & Interactions Wrapper -->
        <div class="interactive-grid">
            <!-- Cookies Card -->
            <div class="card">
                <div class="card-title">Parsed Client Cookies ({len(cookie_jar)})</div>
                {render_cookies(cookie_jar)}
                
                <h3 style="margin-top: 25px; font-size: 1rem; color: #475569;">Set/Test a Cookie (GET)</h3>
                <form method="GET">
                    <div class="form-group">
                        <label for="c_name">Cookie Name</label>
                        <input type="text" id="c_name" name="set_cookie_name" placeholder="e.g., session_id" required>
                    </div>
                    <div class="form-group">
                        <label for="c_val">Cookie Value</label>
                        <input type="text" id="c_val" name="set_cookie_val" placeholder="e.g., xyz123" required>
                    </div>
                    <button type="submit">Send Set-Cookie Directive</button>
                </form>
            </div>

            <!-- POST Tester Card -->
            <div class="card">
                <div class="card-title">Test POST Body Transmission</div>
                <p style="color: #64748b; font-size: 0.9rem; margin-top: 0;">
                    Submit this form to evaluate how your server passes payloads over standard input strings.
                </p>
                <form method="POST" action="{os.environ.get('SCRIPT_NAME', '')}">
                    <div class="form-group">
                        <label for="post_param">Test Parameter Key</label>
                        <input type="text" id="post_param" name="test_key" value="sample_payload_data">
                    </div>
                    <div class="form-group">
                        <label for="post_msg">Custom Message Body</label>
                        <textarea id="post_msg" name="custom_message" rows="3" placeholder="Type a raw text string or structured content..."></textarea>
                    </div>
                    <button type="submit" class="btn-post">Execute POST Request</button>
                </form>
            </div>
        </div>

        <!-- Raw Body Data Card -->
        <div class="card">
            <div class="card-title">Standard Input (Raw POST/PUT Body)</div>
            {f"<pre>{html_escape(body_data)}</pre>" if body_data else "<p style='color: #94a3b8; margin: 0;'>Standard input stream is empty (Content-Length was 0 or unspecified).</p>"}
        </div>

        <!-- Environment Variables Card -->
        <div class="card">
            <div class="card-title">Environment Matrix ({len(os.environ)})</div>
            <table>
                <thead>
                    <tr>
                        <th>Variable Key</th>
                        <th>Assigned Value</th>
                    </tr>
                </thead>
                <tbody>
                    {render_env_variables()}
                </tbody>
            </table>
        </div>
    </div>
</body>
</html>
"""
    print(html_template)

def handle_cookie_submission():
    """Parses GET query string parameters to trigger a Set-Cookie header if requested."""
    query_str = os.environ.get('QUERY_STRING', '')
    if not query_str:
        return None
    
    params = urllib.parse.parse_qs(query_str)
    if 'set_cookie_name' in params and 'set_cookie_val' in params:
        name = params['set_cookie_name'][0]
        val = params['set_cookie_val'][0]
        c = http.cookies.SimpleCookie()
        c[name] = val
        c[name]['path'] = '/'
        c[name]['httponly'] = True
        return c.output()
    return None

def render_cookies(cookie_jar):
    if not cookie_jar:
        return "<p style='color: #94a3b8; margin: 0;'>No cookies detected in the incoming request headers.</p>"
    
    html = "<table><thead><tr><th>Name</th><th>Value</th></tr></thead><tbody>"
    for morph in cookie_jar.values():
        html += f"<tr><td>{html_escape(morph.key)}</td><td>{html_escape(morph.value)}</td></tr>"
    html += "</tbody></table>"
    return html

def render_env_variables():
    critical_keys = {
        'REQUEST_METHOD', 'PATH_INFO', 'PATH_TRANSLATED', 'QUERY_STRING', 
        'CONTENT_LENGTH', 'CONTENT_TYPE', 'SCRIPT_NAME', 'HTTP_COOKIE', 
        'SERVER_SOFTWARE', 'GATEWAY_INTERFACE'
    }
    
    html = ""
    for key in sorted(os.environ.keys()):
        val = os.environ[key]
        css_class = ' class="env-important"' if key in critical_keys else ''
        html += f"<tr><td{css_class}>{html_escape(key)}</td><td>{html_escape(val)}</td></tr>\n"
    return html

def html_escape(text):
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")

if __name__ == '__main__':
    main()
