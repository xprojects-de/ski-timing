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
    <h1>Ski Timming</h1>
    <table class="tabelle" id="results">
        <tr><th>Nr.</th><th>Time (s)</th></tr>
    </table>
    <script>
        setInterval(function() {
          fetch('/data').then(response => response.text()).then(csv => {
            let rows = csv.trim().split('\n');
            let table = "<tr><th>Nr</th><th>Zeit (s)</th></tr>";            
            rows.forEach((time, index) => {
              if(time.length > 0) {
                let newRow = "<tr><td>" + (index + 1) + "</td><td>" + time + " s</td></tr>";
                table = "<tr><th>Nr.</th><th>Time (s)</th></tr>" + newRow + table.replace("<tr><th>Nr</th><th>Zeit (s)</th></tr>", "");
              }
            });
            document.getElementById('results').innerHTML = table;
          });
        }, 2000);
    </script>
</body>
</html>
)rawliteral";