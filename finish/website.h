const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial; text-align: center; margin: 0 auto; }
        .tabelle { width: 90%; margin: auto; border-collapse: collapse; }
        .tabelle td, .tabelle th { border: 1px solid #ddd; padding: 8px; }
        .tabelle th { background-color: #04AA6D; color: white; }
    </style>
</head>
<body>
    <h1>Ski Zeitnahme</h1>
    <table class="tabelle" id="ergebnisse">
        <tr><th>Nr</th><th>Zeit (s)</th></tr>
    </table>
    <script>
        // Funktion zum Laden der Zeiten
        setInterval(function() {
          fetch('/data').then(response => response.text()).then(csv => {
            let rows = csv.split('\n');
            let table = "<tr><th>Nr</th><th>Zeit (s)</th></tr>";
            rows.forEach((row, index) => {
              if(row.trim().length > 0) {
                let cols = row.split(',');
                table += "<tr><td>" + (index+1) + "</td><td>" + cols[1] + "</td></tr>";
              }
            });
            document.getElementById('ergebnisse').innerHTML = table;
          });
        }, 2000);
    </script>
</body>
</html>
)rawliteral";