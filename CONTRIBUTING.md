# Jak pracovat s repem

- `main` je vždy funkční. Nepushuj do něj rozbitý kód.
- Změny dělej ve větvích a do `main` je mergeuj přes pull request. Stačí, aby ho druhý rychle prohlédl.
- Pojmenování větví: `central/<co>`, `node/<co>`, `hw/<co>`, `models/<co>`, `docs/<co>`
  (např. `central/wifi-mqtt`, `node/co2-driver`).
- Commity piš stručně a v jednom jazyce (např. `central: add MQTT reconnect`).
- Každý pracuje primárně ve své složce, takže konflikty vznikají jen výjimečně.
- `firmware/common/` měň jen po domluvě. Je to kontrakt mezi centrálou a node.
- Velké binární soubory (STL/STEP/PDF) commituj jen finální verze, ne každou iteraci.
