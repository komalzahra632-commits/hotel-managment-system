# Hotel Management System (C++)

A C++17 console application for room management, guest records, reservations,
check-in/check-out, billing, and payment reports.

## Build and run

With CMake:

```text
cmake -S . -B build
cmake --build build
```

Run the generated `HotelManagementSystem` executable from the project folder.
The program stores its records in `data/` relative to the current working
directory. The first run creates sample rooms (101 Standard, 102 Deluxe,
201 Executive, and 301 Suite).

Alternatively, compile directly:

```text
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp HotelSystem.cpp -o HotelManagementSystem
```

## Demo login

| Role | Username | Password |
| --- | --- | --- |
| Admin | `admin` | `Admin@123` |
| Receptionist | `receptionist` | `Reception@123` |

The login allows three attempts. Admins can manage rooms and view revenue
reports; both roles can register guests, make and search reservations, check
guests in/out, cancel unpaid bookings, and manage bills. Demo passwords are
hard-coded for coursework demonstrations only and are not secure for production
use.

## Booking and billing

Dates use the `YYYY-MM-DD` format. Reservations use half-open date intervals:
the check-out date is not counted as a night, so a room may be booked by a
different guest starting on the previous guest's check-out date. A checked-in
guest can only be checked out after the bill balance has been settled.
Room rates are copied into a reservation when it is made, so later room price
changes do not alter existing bookings.
Payments, guest records, rooms, and reservations are saved automatically and
can also be saved from the dashboard.

## Data files

- `data/rooms.txt`
- `data/guests.txt`
- `data/reservations.txt`
- `data/payments.txt`
