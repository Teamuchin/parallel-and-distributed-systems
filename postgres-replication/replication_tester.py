import os
import psycopg2
import time
from datetime import datetime


LEADER_CONFIG = {
    "dbname": os.environ.get("LEADER_PGDATABASE", "postgres"),
    "user": os.environ.get("LEADER_PGUSER", "teamuchin"),
    "host": os.environ.get("LEADER_PGHOST", "/var/run/postgresql"),
    "port": os.environ.get("LEADER_PGPORT", "5432"),
}

FOLLOWER_CONFIG = {
    "dbname": os.environ.get("FOLLOWER_PGDATABASE", "postgres"),
    "user": os.environ.get("FOLLOWER_PGUSER", "replication_user"),
    "password": os.environ.get("FOLLOWER_PGPASSWORD"),
    "host": os.environ.get("FOLLOWER_PGHOST", "localhost"),
    "port": os.environ.get("FOLLOWER_PGPORT", "5432"),
}

def get_connection(config, name):
    try:
        return psycopg2.connect(**config)
    except Exception as e:
        print(f"Error connecting to {name}: {e}")
        return None

def data_schema_rep_log_test():

    l_conn = get_connection(LEADER_CONFIG, "LEADER")
    f_conn = get_connection(FOLLOWER_CONFIG, "FOLLOWER")

    if not l_conn or not f_conn:
        print("Nodes are not reachable, exiting...")
        return

    try:
        l_curr = l_conn.cursor()
        f_curr = f_conn.cursor()


        print("\nPerforming INSERT on Leader...")
        start_time = time.perf_counter()
        l_curr.execute("INSERT INTO replication_test (data_content) VALUES (%s) RETURNING id;",
                       ("Visibility Test Data",))

        new_id = l_curr.fetchone()[0]
        l_conn.commit()

        write_time = time.perf_counter()
        visible = False
        attempts = 0
        while not visible and attempts < 100:
            f_curr.execute("SELECT id FROM replication_test WHERE id = %s;", (new_id,))
            if f_curr.fetchone():
                visible_time = time.perf_counter()
                visible = True
            attempts += 1
            time.sleep(0.001)

        if visible:
            latency = (visible_time - write_time) * 1000
            print(f" > Record {new_id} reached Follower in {latency:.2f}ms")
        else:
            print(" > Record not visible on follower after 100 attempts.")





        print("\nPerforming UPDATE on Leader...")
        l_curr.execute("UPDATE replication_test SET data_content = %s WHERE id = %s;",
                       ("Updated Visibility Data", new_id))
        l_conn.commit()
        update_write_time = time.perf_counter()

        version_visible = False
        while not version_visible:
            f_curr.execute("SELECT version_number FROM replication_test WHERE id = %s;", (new_id,))
            res = f_curr.fetchone()
            if res and res[0] == 2:
                update_visible_time = time.perf_counter()
                version_visible = True

        up_latency = (update_visible_time - update_write_time) * 1000
        print(f" >Version 2 reached Follower in {up_latency:.2f}ms")





        print("\nPerforming DELETE on Leader...")
        l_curr.execute("DELETE FROM replication_test WHERE id = %s;", (new_id,))
        l_conn.commit()
        delete_write_time = time.perf_counter()

        visible_time = None
        timeout = 5.0
        poll_start = time.perf_counter()

        while (time.perf_counter() - poll_start) < timeout:
            f_curr.execute("SELECT id FROM replication_test WHERE id = %s", (new_id,))
            result = f_curr.fetchone()

            if result is None:
                visible_time = time.perf_counter()
                break
            time.sleep(0.001)

        if visible_time:
            del_latency = (visible_time - delete_write_time) * 1000
            print(f" > Record {new_id} got deleted on Follower in {del_latency:.2f}ms")
        else:
            print("DELETE visibility timeout reached.")



        l_curr.close()
        f_curr.close()

    finally:
        l_conn.close()
        f_conn.close()

if __name__ == "__main__":
    data_schema_rep_log_test()
