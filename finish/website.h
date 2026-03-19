const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <style>
        body { font-family: Arial; text-align: center; margin: 0 auto; background-color: #f4f4f4; }
        h1 { color: #333; margin-top: 20px; }
        .tabelle { width: 95%; max-width: 600px; margin: 20px auto; border-collapse: collapse; background: white; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        .tabelle td, .tabelle th { border: 1px solid #ddd; padding: 12px; font-size: 1.1em; }
        .tabelle th { background-color: #04AA6D; color: white; position: sticky; top: 0; }
        .tabelle tr:nth-child(even) { background-color: #f9f9f9; }
        .tabelle tr:hover { background-color: #f1f1f1; }
    </style>
</head>
<body>
    <h1>Ski Timing</h1>
    <table class="tabelle" id="results">
        </table>

    <script>
        function updateTable() {
          fetch('/data')
            .then(response => response.text())
            .then(csv => {
              let lines = csv.trim().split('\n').filter(line => line.length > 0);
              
              let tableHTML = "<thead><tr><th>Nr.</th><th>Zeit (s)</th></tr></thead><tbody>";
              
              for (let i = lines.length - 1; i >= 0; i--) {
                let rawValue = lines[i].trim();
                if (rawValue === "") continue;

                let floatTime = parseFloat(rawValue);
                floatTime = floatTime / 1000.0; 

                let formattedTime = isNaN(floatTime) ? rawValue : floatTime.toFixed(2);
                let runNumber = i + 1;

                tableHTML += "<tr><td>" + runNumber + "</td><td><strong>" + formattedTime + " s</strong></td></tr>";
              }
              
              tableHTML += "</tbody>";
              document.getElementById('results').innerHTML = tableHTML;
            })
            .catch(err => console.error("Update Fehler:", err));
        }

        setInterval(updateTable, 2000);
        updateTable(); // Sofort-Start
    </script>
</body>
</html>
)rawliteral";