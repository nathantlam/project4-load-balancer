# Aggie Cloud Solutions

C++17 load balancer simulation for Project 4. Requests wait in a FIFO queue and
are assigned to available web servers. Each request has source and destination
IP addresses, a processing time, and a P (processing) or S (streaming) job type.

## Build and run

Requires g++ and GNU Make. Run these commands from the project directory in a
Linux or Ubuntu WSL terminal:

```sh
make
./loadbalancer
```

The program prompts for the starting server count and number of clock cycles.
The initial queue contains 100 accepted requests per starting server.

Required run:

```sh
./loadbalancer --servers 20 --cycles 100000 --quiet --log logs/required_run.log
```

Use `--verbose --color` to show changing events or `--help` for all options.
Quiet mode prints starting and ending summaries. The log reports queue sizes,
completed and blocked jobs, unfinished work, and server changes.

## Configuration

Edit `config.txt` to change task times, arrival rate, scaling cooldown, server
limits, random seed, and blocked IPv4 CIDR ranges. Use `--config PATH` to select
another configuration file.

The balancer adds one server above 80 queued requests per server and removes one
idle server below 50. A cooldown separates changes, and busy jobs are never
interrupted. If the queue keeps growing, reduce the cooldown or increase the
server limit. If the queue stays empty, consider lowering the minimum server count.

For the bonus, route P and S jobs to separate balancers:

```sh
./loadbalancer --servers 10 --cycles 100000 --bonus
```

In bonus mode, the server count applies to each pool: 10 per pool means 20 total.

## Documentation

Run `make docs` with Doxygen installed, then open `docs/html/index.html`.
The design page is `index.html`, and the required run log is `logs/required_run.log`.
Use `make clean` to remove compiled files.
