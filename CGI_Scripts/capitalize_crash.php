<?php
header('Content-Type: text/plain');
header('Content-Length: 50');

// Disable PHP's internal output buffering so data flows instantly
while (ob_get_level()) {
    ob_end_clean();
}

// 1. Get our input data
if (isset($_GET['text'])) {
    $inputData = $_GET['text'];
} else {
    $inputData = file_get_contents('php://input');
}

if (!empty($inputData)) {
    // 2. Capitalize the input
    $capitalized = strtoupper($inputData);
    
    // 3. Grab exactly the first 4096 bytes (the first 4KB chunk)
    $firstChunk = substr($capitalized, 0, 50);
    echo $firstChunk;
    
    // 4. Force Nginx/PHP-FPM to flush this chunk over the network right now
    flush(); 
    
    // 5. CRASH! Kill the script immediately. 
    // This stops the rest of the text from processing and terminates the connection.
    trigger_error("Intentional Testing Crash: First 4KB chunk sent.", E_USER_ERROR);

} else {
    echo "Error: Please provide some text to capitalize.";
}
?>