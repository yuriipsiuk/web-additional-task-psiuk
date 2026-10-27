Basic HTTP Server (Socket API)
1. Get запит з параметрами
   curl "http://localhost:8080/page?id=777"
2. Get запит без параметрів
   curl "http://localhost:8080/page"
3. POST-запит
   curl -X POST http://localhost:8080/ -H "Content-Type: application/json" -d "{\"score\": 4, \"name\": "yurii"}"
