import os
import psycopg2
import time
from datetime import datetime


LEADER_CONFIG = {
    "dbname": os.environ.get("LEADER_PGDATABASE", "postgres"),
    "user": os.environ.get("LEADER_PGUSER", "postgres"),
    "host": os.environ.get("LEADER_PGHOST", "/var/run/postgresql"),
    "port": os.environ.get("LEADER_PGPORT", "5432"),
}

FOLLOWER_CONFIG = {
    "dbname": os.environ.get("FOLLOWER_PGDATABASE", "postgres"),
    "user": os.environ.get("FOLLOWER_PGUSER", "postgres"),
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

def run_vet_replication_test():
    l_conn = get_connection(LEADER_CONFIG, "LEADER")
    f_conn = get_connection(FOLLOWER_CONFIG, "FOLLOWER")

    if not l_conn or not f_conn:
        print("Database nodes are not reachable. Exiting...")
        return

    try:
        l_curr = l_conn.cursor()
        f_curr = f_conn.cursor()

        print("=========================================================")
        print("REPLICATION TEST EXPERIMENTS")
        print("=========================================================")






        print("\n[TEST 1] Performing INSERT on Leader...")
        
        insert_query = """
            INSERT INTO owners (first_name, last_name, phone, email) 
            VALUES (%s, %s, %s, %s) 
            RETURNING owner_id;
        """
        
        l_curr.execute(insert_query, ("Jane", "ClinicPatient", "555-0122", "jane.vet@example.com"))
        new_id = l_curr.fetchone()[0]
        l_conn.commit()
        
        write_time = time.perf_counter()
        visible = False
        attempts = 0
        
        while not visible and attempts < 100:
            f_curr.execute("SELECT owner_id FROM owners WHERE owner_id = %s;", (new_id,))
            if f_curr.fetchone():
                visible_time = time.perf_counter()
                visible = True
            attempts += 1
            time.sleep(0.001)

        if visible:
            latency = (visible_time - write_time) * 1000
            print(f" > Success: New Owner ID {new_id} reached Follower in {latency:.2f}ms")
        else:
            print(" > Timeout: Record not visible on follower after 100 attempts.")






        print("\n[TEST 2] Performing UPDATE on Leader...")
        
        update_query = "UPDATE owners SET phone = %s WHERE owner_id = %s;"
        l_curr.execute(update_query, ("555-9999", new_id))
        l_conn.commit()
        
        update_write_time = time.perf_counter()
        version_visible = False
        update_visible_time = None

        poll_start = time.perf_counter()
        while not version_visible and (time.perf_counter() - poll_start) < 5.0:
            f_curr.execute("SELECT version_number FROM owners WHERE owner_id = %s;", (new_id,))
            res = f_curr.fetchone()
            if res and res[0] == 2:
                update_visible_time = time.perf_counter()
                version_visible = True
            time.sleep(0.001)

        if update_visible_time:
            up_latency = (update_visible_time - update_write_time) * 1000
            print(f" > Success: Version 2 reached Follower in {up_latency:.2f}ms")
        else:
            print(" > Timeout: Version 2 not visible on Follower within 5 seconds.")







        print("\n[TEST 3] Performing DELETE on Leader...")
        
        l_curr.execute("DELETE FROM owners WHERE owner_id = %s;", (new_id,))
        l_conn.commit()
        delete_write_time = time.perf_counter()

        delete_visible_time = None
        timeout = 5.0
        poll_start = time.perf_counter()

        while (time.perf_counter() - poll_start) < timeout:
            f_curr.execute("SELECT owner_id FROM owners WHERE owner_id = %s", (new_id,))
            result = f_curr.fetchone()
            
            if result is None:
                delete_visible_time = time.perf_counter()
                break
            time.sleep(0.001)

        if delete_visible_time:
            del_latency = (delete_visible_time - delete_write_time) * 1000
            print(f" > Success: Owner ID {new_id} erasure confirmed on Follower in {del_latency:.2f}ms")
        else:
            print(" > Timeout: DELETE visibility verification exceeded boundary limit.")

        l_curr.close()
        f_curr.close()

    finally:
        l_conn.close()
        f_conn.close()





def run_consistency_experiments():
    l_conn = get_connection(LEADER_CONFIG, "LEADER")
    f_conn = get_connection(FOLLOWER_CONFIG, "FOLLOWER")

    if not l_conn or not f_conn:
        print("Database nodes are not reachable. Exiting...")
        return

    try:
        l_curr = l_conn.cursor()
        f_curr = f_conn.cursor()

        print("=========================================================")
        print(" REPLICATION CONSISTENCY EXPERIMENTS")
        print("=========================================================")




        print("\n[EXPERIMENT 1] Eventual Consistency")
        print(" -> Action: Writing new owner to Leader and polling Follower...")
        
        l_curr.execute(
            "INSERT INTO owners (first_name, last_name, phone) VALUES (%s, %s, %s) RETURNING owner_id;",
            ("Eventual", "ConsistencyTest", "555-0001")
        )
        exp1_id = l_curr.fetchone()[0]
        l_conn.commit()
        
        start_time = time.perf_counter()
        visible = False
        end_time = None

        while not visible and (time.perf_counter() - start_time) < 5.0:
            f_curr.execute("SELECT owner_id FROM owners WHERE owner_id = %s;", (exp1_id,))
            if f_curr.fetchone():
                end_time = time.perf_counter()
                visible = True
            time.sleep(0.001) # 1ms polling

        if end_time:
            lag = (end_time - start_time) * 1000
            print(f" -> Result: Follower converged in {lag:.2f}ms. Eventual consistency achieved.")
        else:
            print(" -> Timeout: Follower did not converge within 5 seconds.")





        print("\n[EXPERIMENT 2] Monotonic Reads")
        print(" -> Action: Executing 5 rapid sequential updates on Leader...")
        
        for i in range(2, 7):
            l_curr.execute("UPDATE owners SET phone = %s WHERE owner_id = %s;", (f"555-000{i}", exp1_id))
            l_conn.commit()
            
        print(" -> Action: Polling Follower rapidly to check for out-of-order versions...")
        read_versions = []
        timeout = time.perf_counter() + 2.0 # 2 second observation window
        
        while time.perf_counter() < timeout:
            f_curr.execute("SELECT version_number FROM owners WHERE owner_id = %s;", (exp1_id,))
            res = f_curr.fetchone()
            if res and (not read_versions or read_versions[-1] != res[0]):
                read_versions.append(res[0])
            time.sleep(0.01)
            
        print(f" -> Observed Follower Version Sequence: {read_versions}")
        is_monotonic = all(x <= y for x, y in zip(read_versions, read_versions[1:]))
        if is_monotonic:
            print(" -> Result: Sequence strictly ascending. Monotonic Reads maintained.")
        else:
            print(" -> Result: Backward read detected! Monotonic Reads violated.")






        print("\n[EXPERIMENT 3] Read-After-Write Consistency")
        print(" -> Action: Client writes to Leader and immediately reads from Leader...")
        
        l_curr.execute(
            "INSERT INTO owners (first_name, last_name, phone) VALUES (%s, %s, %s) RETURNING owner_id;",
            ("ReadAfter", "WriteTest", "555-0002")
        )
        exp3_id = l_curr.fetchone()[0]
        l_conn.commit()
        write_stamp = time.perf_counter()

        l_curr.execute("SELECT owner_id FROM owners WHERE owner_id = %s;", (exp3_id,))
        if l_curr.fetchone():
            print(" -> Result (Leader): Immediate read successful! Read-After-Write maintained for writing client.")

        f_curr.execute("SELECT owner_id FROM owners WHERE owner_id = %s;", (exp3_id,))
        if not f_curr.fetchone():
            print(" -> Result (Follower): Immediate read FAILED. Other clients experience asynchronous delay.")
            
        catchup_start = time.perf_counter()
        while (time.perf_counter() - catchup_start) < 5.0:
            f_curr.execute("SELECT owner_id FROM owners WHERE owner_id = %s;", (exp3_id,))
            if f_curr.fetchone():
                delay = (time.perf_counter() - write_stamp) * 1000
                print(f" -> Delay recorded: It took {delay:.2f}ms for data to reach other clients.")
                break
            time.sleep(0.001)






        print("\n[SCENARIO] Concurrent Writes")
        print(" -> Action: Blasting 10 inserts to Leader in quick succession...")
        
        batch_ids = []
        for i in range(10):
            l_curr.execute(
                "INSERT INTO owners (first_name, last_name, email) VALUES (%s, %s, %s) RETURNING owner_id;",
                (f"ConcurrentUser_{i}", "Test", f"user{i}@test.com")
            )
            batch_ids.append(l_curr.fetchone()[0])
            l_conn.commit()
        
        print(f" -> Intended Order (Leader): {batch_ids}")
        time.sleep(0.5)
        
        f_curr.execute(
            "SELECT owner_id FROM owners WHERE owner_id = ANY(%s) ORDER BY last_updated ASC, owner_id ASC;",
            (batch_ids,)
        )
        follower_order = [row[0] for row in f_curr.fetchall()]
        print(f" -> Received Order (Follower): {follower_order}")
        
        if batch_ids == follower_order:
            print(" -> Result: Follower replicated concurrent writes in the exact correct sequence.")
        else:
            print(" -> Result: Ordering discrepancy detected due to asynchronous replication limits!")

        print("\n=========================================================")
        print(" EXPERIMENTS COMPLETE. Cleaning up database...")
        l_curr.execute("DELETE FROM owners WHERE last_name IN ('ConsistencyTest', 'WriteTest', 'Test');")
        l_conn.commit()

        l_curr.close()
        f_curr.close()

    finally:
        l_conn.close()
        f_conn.close()

if __name__ == "__main__":
    run_vet_replication_test()
    run_consistency_experiments()
