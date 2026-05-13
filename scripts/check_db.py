import psycopg2
try:
    conn = psycopg2.connect(
        host="localhost",
        database="ddos_detection_db",
        user="postgres",
        password="qwerty"
    )
    cur = conn.cursor()
    cur.execute("SELECT table_name FROM information_schema.tables WHERE table_schema='public';")
    tables = cur.fetchall()
    print("Tables in database:")
    for table in tables:
        print(f"- {table[0]}")
    
    cur.execute("SELECT COUNT(*) FROM security_events;")
    print(f"Security events count: {cur.fetchone()[0]}")
    
    cur.execute("SELECT COUNT(*) FROM sessions;")
    print(f"Sessions count: {cur.fetchone()[0]}")
    
    cur.close()
    conn.close()
except Exception as e:
    print(f"Error: {e}")
