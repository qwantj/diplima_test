import psycopg2
try:
    conn = psycopg2.connect(host="localhost", database="ddos_detection_db", user="postgres", password="qwerty")
    cur = conn.cursor()
    cur.execute("SELECT id, interface_name, start_time FROM sessions ORDER BY id DESC LIMIT 5;")
    rows = cur.fetchall()
    print("Latest sessions:")
    for row in rows:
        print(f"ID: {row[0]}, Interface: {row[1]}, Start: {row[2]}")
    cur.close()
    conn.close()
except Exception as e:
    print(f"Error: {e}")
