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
        .btn { padding: 12px 24px; font-size: 1.1em; margin: 15px auto; border: none; border-radius: 5px; cursor: pointer; display: block; width: 95%; max-width: 600px; }
        .btn-danger { background-color: #f44336; color: white; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }
        .btn-danger:hover { background-color: #da190b; }
        .btn-danger:disabled { background-color: #aaa; cursor: not-allowed; }
    </style>
</head>
<body>
    <h1>Ski Timing</h1>
    <button class="btn btn-danger" id="discardBtn" onclick="discardStarter()">Sturz: Läufer aus Queue entfernen</button>
    <table class="tabelle" id="results">
        </table>

    <script>
        let isUpdating = false;  // Flag um mehrfache gleichzeitige Requests zu verhindern

        function updateTable() {
          // Verhindere mehrfache parallele Requests
          if (isUpdating) {
            console.log("Update already in progress, skipping...");
            return;
          }

          isUpdating = true;

          // Timeout für fetch() - bricht Request nach 5 Sekunden ab
          const controller = new AbortController();
          const timeoutId = setTimeout(() => controller.abort(), 5000);

          fetch('/data', { signal: controller.signal })
            .then(response => {
              clearTimeout(timeoutId);
              return response.text();
            })
            .then(csv => {
              let lines = csv.trim().split('\n').filter(line => line.length > 0);
              
              let tableHTML = "<thead><tr><th>Nr.</th><th>Zeit (s)</th></tr></thead><tbody>";
              
              for (let i = lines.length - 1; i >= 0; i--) {
                let line = lines[i].trim();
                if (line === "") continue;

                // Parse CSV format: id,time
                let parts = line.split(',');
                if (parts.length >= 2) {
                  let runNumber = parts[0];
                  let floatTime = parseFloat(parts[1]);
                  floatTime = floatTime / 1000.0;
                  let formattedTime = isNaN(floatTime) ? parts[1] : floatTime.toFixed(2);

                  tableHTML += "<tr><td>" + runNumber + "</td><td><strong>" + formattedTime + " s</strong></td></tr>";
                } else {
                  // Fallback für altes Format (nur Zeit)
                  let floatTime = parseFloat(line);
                  floatTime = floatTime / 1000.0;
                  let formattedTime = isNaN(floatTime) ? line : floatTime.toFixed(2);
                  let runNumber = i + 1;

                  tableHTML += "<tr><td>" + runNumber + "</td><td><strong>" + formattedTime + " s</strong></td></tr>";
                }
              }
              
              tableHTML += "</tbody>";
              document.getElementById('results').innerHTML = tableHTML;
            })
            .catch(err => {
              if (err.name === 'AbortError') {
                console.error("Request timeout nach 5s");
              } else {
                console.error("Update Fehler:", err);
              }
            })
            .finally(() => {
              clearTimeout(timeoutId);
              isUpdating = false;
            });
        }

        setInterval(updateTable, 3000);
        updateTable();

        function discardStarter() {
          if (!confirm("Älteste Startzeit verwerfen?\n(z.B. wegen Sturz oder Fehlstart)\n\nDiese Aktion kann nicht rückgängig gemacht werden!")) return;
          const btn = document.getElementById('discardBtn');
          btn.disabled = true;
          fetch('/discard')
            .then(response => {
              if (response.ok) {
                updateTable();
              } else {
                response.text().then(msg => alert("Fehler: " + msg));
              }
            })
            .catch(err => {
              console.error("Netzwerkfehler:", err);
              alert("Verbindung zum Zeitmessgerät verloren!");
            })
            .finally(() => {
              btn.disabled = false;
            });
        }
    </script>
</body>
</html>
)rawliteral";