<?php
// receiver.php
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // Načte surová data z těla požadavku
    $data = file_get_contents('php://input');
    
    // Přidá datum a čas pro přehlednost
    $timestamp = date("Y-m-d H:i:s");
    $entry = "[$timestamp]\n" . $data . "\n-------------------\n";

    // Uloží do souboru logs.txt (vytvoří ho, pokud neexistuje)
    file_put_contents('logs.txt', $entry, FILE_APPEND);
    
    echo "OK";
}
?>