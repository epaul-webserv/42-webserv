<?php
// Set the response type to plain text
header('Content-Type: text/plain');

// 1. Check if data was passed in the URL (GET query string, e.g., ?text=hello)
if (isset($_GET['text'])) {
    $inputData = $_GET['text'];
} 
// 2. Fallback: Check if data was passed in the raw request body
else {
    $inputData = file_get_contents('php://input');
}

// 3. If we got data, capitalize it and send it back. Otherwise, show an error.
if (!empty($inputData)) {
    echo strtoupper($inputData);
} else {
    echo "Error: Please provide some text to capitalize. Use ?text=yourtext or send a request body.";
}
?>