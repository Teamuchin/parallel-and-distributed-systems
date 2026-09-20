# Parallel & Distributed Systems Coursework

A shared-memory synchronisation benchmark written with OpenMP, and a PostgreSQL leader–replica replication setup with a failure-detection tester.

**Course:** CENG444 (parallel programming) and CENG465 (distributed data)  
**Institution:** İzmir Institute of Technology (IYTE) — İzmir, Türkiye

## OpenMP mutual exclusion

`openmp-mutual-exclusion/` benchmarks the critical-section problem. Each variant generates a
randomised array and measures how long the critical section takes under a different
synchronisation strategy: `mcs.c` is the baseline, with `mcs1.c`, `mcs2.c`, `mcs3.c` and the
final `mcsfinal.c` as successive revisions.

`day17.py` sits alongside it — a `heapq`-based shortest-path solution to Advent of Code 2024
day 17, written during the same period but unrelated to the OpenMP work.

```bash
gcc -fopenmp mcsfinal.c -o mcs && ./mcs
```

## PostgreSQL replication

`postgres-replication/` drives a leader–replica setup. `prj.py` holds the leader and follower
connection settings, writes rows to the leader and polls the follower to measure replication
lag. `replication_tester.py` probes the link and records failures, which is how the recovery
behaviour was checked.

Connection settings come from the environment, so no credentials are committed:

```bash
export LEADER_PGHOST=/var/run/postgresql   LEADER_PGUSER=postgres
export FOLLOWER_PGHOST=<replica-host>      FOLLOWER_PGUSER=replication_user
export FOLLOWER_PGPASSWORD=<password>
python prj.py
```

The leader defaults to the local Unix socket (`/var/run/postgresql`, database `postgres`)
with a follower configured to stream from it.

---

Submitted reports, worksheets and lecture material are archived outside this
repository rather than committed, so the repo stays code-only.
