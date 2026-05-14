# Hospital Management System (HMS) - C++/SFML GUI

## Overview

This Hospital Management System is a desktop-based application developed as a semester project for Programming Fundamentals (PF). While the original project scope was limited to a terminal-based interface, this version utilizes the **Simple and Fast Multimedia Library (SFML)** to provide a comprehensive Graphical User Interface (GUI).

The system facilitates the management of healthcare data, including patient registration, medical staff directories, appointment scheduling, and financial tracking, all while ensuring data persistence through file-based storage.

## Development Motivation: Terminal to GUI

A primary objective of this project was to explore software development beyond the command line. By integrating SFML, the project evolved to include:

* **Event-Driven Architecture:** Handling real-time user inputs, mouse interactions, and window events.
* **Modern Interface Design:** A custom-built dashboard featuring interactive components, organized data grids, and state-based navigation.
* **Synchronous UI Updates:** Real-time visual feedback for data entry and system notifications.

## Core Functionality

### 1. Administrative Modules

* **Patient Records:** Full management of patient demographics and medical history.
* **Doctor Directory:** Categorized records of medical professionals by specialty and experience level.
* **Appointment Scheduling:** A relational system that links patient IDs with doctor availability and specific time slots.

### 2. Clinical & Financial Operations

* **Treatment Logging:** Detailed tracking of medical procedures and treatments assigned to specific patients.
* **Billing System:** Automated generation of bills with tracking for payment status (Paid/Unpaid) and procedural costs.

### 3. Analytics & Search Tools

* **Multi-Parameter Search:** Substring-based search functionality for locating patients and doctors by name, ID, or specialty.
* **Aggregated Reports:** A high-level overview dashboard displaying hospital-wide metrics, including total revenue and outstanding payments.
* **Sorting Algorithms:** Logic-based sorting of doctor lists based on professional experience.

## Technical Specifications

* **Language:** C++17
* **Library:** SFML (Graphics, Window, System)
* **Data Persistence:** File I/O operations using flat-file storage (`.txt`).
* **Memory Management:** Implementation of pointer arithmetic for efficient data handling.
* **Software Design:** Modular architecture for scalability and separation of concerns.

## Installation and Setup

### Prerequisites

* C++ Compiler (GCC/MinGW)
* SFML Library 2.5.1+

### Execution

1. Clone the repository.
2. Link the SFML dependencies in your build environment.
3. Compile and execute `main.cpp`.

## Author

**Muhammad Shayan Shahid** Computer Engineering Student, FAST National University (NUCES)
