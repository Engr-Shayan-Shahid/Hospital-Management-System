// =============================================================================
//  Hospital Management System - SFML GUI - FINAL (Phase E)
//  All modules: Login, Dashboard, Patients, Doctors, Appointments,
//               Treatments, Billing, Search & Reports
//
//  What's new vs Phase D
//  ---------------------
//  - Search & Reports hub with 5 tools (matches your original menu)
//      1. Search patients by ID or name (substring, case-insensitive)
//      2. Search doctors by ID or speciality
//      3. Treatments by doctor (joins appointments + treatments)
//      4. Sort doctors by experience (ascending, mutates doctor list)
//      5. Overview / dashboard report (revenue, outstanding, counts)
//  - All search results presented in the same table style as the lists
// =============================================================================

#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <cstdint>
#include <cctype>
#include <iomanip>
#include <algorithm>
#include <ctime>

// =============================================================================
//  Theme
// =============================================================================
namespace Theme {
    const sf::Color BG_DARK        (15,  23,  42);
    const sf::Color BG_PANEL       (30,  41,  59);
    const sf::Color BG_PANEL_HOVER (45,  55,  72);
    const sf::Color BG_INPUT       (51,  65,  85);
    const sf::Color BG_ROW_ALT     (24,  33,  51);
    const sf::Color BG_ROW_SELECTED(20,  90,  85);

    const sf::Color ACCENT          (20, 184, 166);
    const sf::Color ACCENT_HOVER    (45, 212, 191);
    const sf::Color ACCENT_DARK     (15, 118, 110);

    const sf::Color TEXT_PRIMARY   (248, 250, 252);
    const sf::Color TEXT_SECONDARY (148, 163, 184);
    const sf::Color TEXT_MUTED     (100, 116, 139);

    const sf::Color SUCCESS        ( 34, 197,  94);
    const sf::Color SUCCESS_DARK   ( 21, 128,  61);
    const sf::Color WARNING        (250, 204,  21);
    const sf::Color WARNING_DARK   (202, 138,   4);
    const sf::Color DANGER         (239,  68,  68);
    const sf::Color DANGER_HOVER   (248, 113, 113);
    const sf::Color DANGER_DARK    (185,  28,  28);
    const sf::Color INFO           ( 56, 189, 248);

    const sf::Color BORDER         (51,  65,  85);
    const sf::Color BORDER_FOCUS   (20, 184, 166);

    const sf::Color OVERLAY        (0,   0,   0, 180);
}

// =============================================================================
//  Data models
// =============================================================================
struct Patient {
    int patientid = 0;
    std::string name;
    int age = 0;
    std::string gender;
    std::string contact;
    double balance = 0.0;
};

struct Doctor {
    int docid = 0;
    std::string name;
    std::string speciality;
    int experience = 0;
};

struct Appointment {
    int patientid = 0;
    int doctorid = 0;
    std::string date;
    std::string time;
};

struct Treatment {
    int patientid = 0;
    std::string description;
    double cost = 0.0;
    bool paid = false;
};

std::vector<Patient>     patients;
std::vector<Doctor>      doctors;
std::vector<Appointment> appointments;
std::vector<Treatment>   treatments;

const double CONSULTATION_FEE = 500.0;

// =============================================================================
//  Screen state
// =============================================================================
enum class Screen {
    Login, MainMenu,
    PatientList, PatientAdd, PatientEdit,
    DoctorList,  DoctorAdd,  DoctorEdit,
    AppointmentList, AppointmentAdd, AppointmentEdit,
    TreatmentList, TreatmentAdd, TreatmentEdit,
    Billing,
    SearchMenu,
    SearchPatient, SearchDoctor, TreatmentsByDoctor, SortDoctors, ReportOverview,
    Exit
};

Screen currentScreen = Screen::Login;
sf::Font font;

int selectedPatientIndex   = -1;
int selectedDoctorIndex    = -1;
int selectedApptIndex      = -1;
int selectedTreatmentIndex = -1;

// =============================================================================
//  Modal + Toast
// =============================================================================
struct Modal {
    bool active = false;
    std::string title; std::string message;
    bool isConfirm = false;
    std::function<void()> onConfirm;
    sf::Color accent = sf::Color(20, 184, 166);
};
Modal modal;

struct Toast {
    std::string text; sf::Color color = sf::Color(34, 197, 94);
    sf::Clock clock; bool active = false; float duration = 3.0f;
};
Toast toast;

void showToast(const std::string& msg, sf::Color color = Theme::SUCCESS) {
    toast.text = msg; toast.color = color;
    toast.clock.restart(); toast.active = true;
}
void showConfirm(const std::string& title, const std::string& message,
                 std::function<void()> onConfirm,
                 sf::Color accent = Theme::DANGER) {
    modal.active = true; modal.title = title; modal.message = message;
    modal.isConfirm = true; modal.onConfirm = onConfirm; modal.accent = accent;
}

// =============================================================================
//  Helpers
// =============================================================================
bool validContact(const std::string& c) {
    if (c.length() != 11) return false;
    for (char ch : c) if (!std::isdigit((unsigned char)ch)) return false;
    return true;
}
std::string fixGender(const std::string& g) {
    if (g == "M" || g == "m") return "Male";
    if (g == "F" || g == "f") return "Female";
    return g;
}
std::string fixDate(const std::string& date) {
    if (date.length() == 10 && date[4] == '-') {
        return date.substr(5, 2) + "-" + date.substr(8, 2) + "-" + date.substr(0, 4);
    }
    return date;
}
bool validDate(const std::string& d) {
    if (d.length() != 10) return false;
    if (d[2] != '-' || d[5] != '-') return false;
    for (int i : {0,1,3,4,6,7,8,9}) if (!std::isdigit((unsigned char)d[i])) return false;
    int month = std::stoi(d.substr(0, 2));
    int day   = std::stoi(d.substr(3, 2));
    int year  = std::stoi(d.substr(6, 4));
    return (month >= 1 && month <= 12 && day >= 1 && day <= 31 && year >= 1900 && year <= 2100);
}
std::string fixTime(const std::string& t) {
    for (char c : t) if (c == 'A' || c == 'P') return t;
    int colonpos = -1;
    for (size_t i = 0; i < t.length(); ++i) if (t[i] == ':') { colonpos = (int)i; break; }
    if (colonpos <= 0) return t;
    int hour = (colonpos == 1) ? (t[0] - '0') : ((t[0] - '0') * 10 + (t[1] - '0'));
    std::string minutes;
    for (size_t i = colonpos + 1; i < t.length(); ++i) minutes += t[i];
    std::string period;
    if (hour == 0 || hour == 24)     { hour = 12; period = "AM"; }
    else if (hour == 12)             { period = "PM"; }
    else if (hour > 12)              { hour -= 12; period = "PM"; }
    else                             { period = "AM"; }
    std::string hourstr;
    if (hour >= 10) { hourstr += (char)('0' + hour/10); hourstr += (char)('0' + hour%10); }
    else            { hourstr += (char)('0' + hour); }
    return hourstr + ":" + minutes + " " + period;
}
bool validTime(const std::string& t) {
    if (t.empty()) return false;
    if (t.find(':') == std::string::npos) return false;
    return true;
}
std::string todayDate() {
    std::time_t t = std::time(nullptr);
    std::tm* lt = std::localtime(&t);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d-%02d-%04d",
                  lt->tm_mon + 1, lt->tm_mday, lt->tm_year + 1900);
    return buf;
}
std::string moneyStr(double v) {
    std::stringstream ss; ss << std::fixed << std::setprecision(2) << v;
    return ss.str();
}
std::string toLower(const std::string& s) {
    std::string r; r.reserve(s.size());
    for (char c : s) r += (char)std::tolower((unsigned char)c);
    return r;
}
bool containsCI(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    std::string a = toLower(haystack), b = toLower(needle);
    return a.find(b) != std::string::npos;
}

// =============================================================================
//  File I/O — patients
// =============================================================================
void cleanPatients() {
    std::ifstream in("patients.txt"); if (!in) return;
    std::vector<std::string> valid; std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string sid, name, sage, gender, contact, sbal;
        std::getline(ss, sid, '#'); std::getline(ss, name, '#');
        std::getline(ss, sage, '#'); std::getline(ss, gender, '#');
        std::getline(ss, contact, '#'); std::getline(ss, sbal, '#');
        if (sid.empty() || name.empty() || contact.empty()) continue;
        bool ok = true;
        for (char c : sid) if (!std::isdigit((unsigned char)c)) ok = false;
        if (!ok) continue;
        if (!validContact(contact)) continue;
        gender = fixGender(gender);
        valid.push_back(sid+"#"+name+"#"+sage+"#"+gender+"#"+contact+"#"+sbal);
    }
    in.close();
    std::vector<bool> dup(valid.size(), false);
    for (size_t i = 0; i < valid.size(); ++i) {
        std::string id1 = valid[i].substr(0, valid[i].find('#'));
        for (size_t j = i+1; j < valid.size(); ++j) {
            std::string id2 = valid[j].substr(0, valid[j].find('#'));
            if (id1 == id2) { dup[i] = true; dup[j] = true; }
        }
    }
    std::ofstream out("patients.txt");
    for (size_t i = 0; i < valid.size(); ++i) if (!dup[i]) out << valid[i] << "\n";
}
void loadPatients() {
    patients.clear();
    std::ifstream f("patients.txt"); if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string sid, name, sage, gender, contact, sbal;
        std::getline(ss, sid, '#'); std::getline(ss, name, '#');
        std::getline(ss, sage, '#'); std::getline(ss, gender, '#');
        std::getline(ss, contact, '#'); std::getline(ss, sbal, '#');
        if (sid.empty() || name.empty()) continue;
        Patient p;
        try {
            p.patientid = std::stoi(sid); p.name = name;
            p.age = sage.empty() ? 0 : std::stoi(sage);
            p.gender = gender; p.contact = contact;
            p.balance = sbal.empty() ? 0.0 : std::stod(sbal);
            patients.push_back(p);
        } catch (...) {}
    }
}
void savePatients() {
    std::ofstream f("patients.txt");
    for (const auto& p : patients)
        f << p.patientid << "#" << p.name << "#" << p.age << "#"
          << p.gender << "#" << p.contact << "#" << p.balance << "#\n";
}

// =============================================================================
//  File I/O — doctors
// =============================================================================
void cleanDoctors() {
    std::ifstream in("doctors.txt"); if (!in) return;
    std::vector<std::string> valid; std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string sid, name, spec, sexp;
        std::getline(ss, sid, '#'); std::getline(ss, name, '#');
        std::getline(ss, spec, '#'); std::getline(ss, sexp, '#');
        if (sid.empty() || name.empty()) continue;
        bool ok = true;
        for (char c : sid) if (!std::isdigit((unsigned char)c)) ok = false;
        if (!ok) continue;
        valid.push_back(sid+"#"+name+"#"+spec+"#"+sexp);
    }
    in.close();
    std::vector<bool> dup(valid.size(), false);
    for (size_t i = 0; i < valid.size(); ++i) {
        std::string id1 = valid[i].substr(0, valid[i].find('#'));
        for (size_t j = i+1; j < valid.size(); ++j) {
            std::string id2 = valid[j].substr(0, valid[j].find('#'));
            if (id1 == id2) { dup[i] = true; dup[j] = true; }
        }
    }
    std::ofstream out("doctors.txt");
    for (size_t i = 0; i < valid.size(); ++i) if (!dup[i]) out << valid[i] << "\n";
}
void loadDoctors() {
    doctors.clear();
    std::ifstream f("doctors.txt"); if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string sid, name, spec, sexp;
        std::getline(ss, sid, '#'); std::getline(ss, name, '#');
        std::getline(ss, spec, '#'); std::getline(ss, sexp, '#');
        if (sid.empty() || name.empty()) continue;
        Doctor d;
        try {
            d.docid = std::stoi(sid); d.name = name;
            d.speciality = spec;
            d.experience = sexp.empty() ? 0 : std::stoi(sexp);
            doctors.push_back(d);
        } catch (...) {}
    }
}
void saveDoctors() {
    std::ofstream f("doctors.txt");
    for (const auto& d : doctors)
        f << d.docid << "#" << d.name << "#" << d.speciality << "#" << d.experience << "#\n";
}

// =============================================================================
//  File I/O — appointments
// =============================================================================
void cleanAppointments() {
    std::ifstream in("appointments.txt"); if (!in) return;
    std::vector<std::string> lines; std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string spid, sdid, date, stime;
        std::getline(ss, spid, '#'); std::getline(ss, sdid, '#');
        std::getline(ss, date, '#'); std::getline(ss, stime, '#');
        if (spid.empty() || sdid.empty()) continue;
        date = fixDate(date); stime = fixTime(stime);
        lines.push_back(spid+"#"+sdid+"#"+date+"#"+stime);
    }
    in.close();
    std::ofstream out("appointments.txt");
    for (const auto& l : lines) out << l << "\n";
}
void loadAppointments() {
    appointments.clear();
    std::ifstream f("appointments.txt"); if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string spid, sdid, date, stime;
        std::getline(ss, spid, '#'); std::getline(ss, sdid, '#');
        std::getline(ss, date, '#'); std::getline(ss, stime, '#');
        if (spid.empty() || sdid.empty()) continue;
        Appointment a;
        try {
            a.patientid = std::stoi(spid); a.doctorid = std::stoi(sdid);
            a.date = date; a.time = stime;
            appointments.push_back(a);
        } catch (...) {}
    }
}
void saveAppointments() {
    std::ofstream f("appointments.txt");
    for (const auto& a : appointments)
        f << a.patientid << "#" << a.doctorid << "#" << a.date << "#" << a.time << "#\n";
}

// =============================================================================
//  File I/O — treatments & bills
// =============================================================================
void loadTreatments() {
    treatments.clear();
    std::ifstream f("treatments.txt"); if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string spid, desc, scost, spaid;
        std::getline(ss, spid, '#'); std::getline(ss, desc, '#');
        std::getline(ss, scost, '#'); std::getline(ss, spaid, '#');
        if (spid.empty()) continue;
        Treatment t;
        try {
            t.patientid = std::stoi(spid); t.description = desc;
            t.cost = scost.empty() ? 0.0 : std::stod(scost);
            t.paid = (spaid == "true" || spaid == "Paid");
            treatments.push_back(t);
        } catch (...) {}
    }
}
void saveTreatments() {
    std::ofstream f("treatments.txt");
    for (const auto& t : treatments)
        f << t.patientid << "#" << t.description << "#" << t.cost << "#"
          << (t.paid ? "true" : "false") << "\n";
}
void saveBills() {
    std::ofstream f("bills.txt");
    for (const auto& t : treatments) {
        f << t.patientid << "#" << t.cost << "#"
          << (t.paid ? "Paid" : "Unpaid") << "\n";
    }
}

// =============================================================================
//  Lookup helpers
// =============================================================================
std::string getPatientName(int id) {
    for (const auto& p : patients) if (p.patientid == id) return p.name;
    return "(unknown)";
}
std::string getDoctorName(int id) {
    for (const auto& d : doctors) if (d.docid == id) return d.name;
    return "(unknown)";
}
int findPatientIndex(int id) {
    for (size_t i = 0; i < patients.size(); ++i) if (patients[i].patientid == id) return (int)i;
    return -1;
}
bool patientExists(int id) { return findPatientIndex(id) != -1; }
bool doctorExists(int id) {
    for (const auto& d : doctors) if (d.docid == id) return true;
    return false;
}
int findPendingTreatment(int patientid) {
    for (size_t i = 0; i < treatments.size(); ++i)
        if (treatments[i].patientid == patientid && !treatments[i].paid)
            return (int)i;
    return -1;
}

// =============================================================================
//  UI: Button
// =============================================================================
class Button {
public:
    sf::RectangleShape box;
    sf::Text label;
    bool hovered = false; bool pressed = false;
    sf::Color bgIdle, bgHover, bgPressed;

    Button(const std::string& text, sf::Vector2f position, sf::Vector2f size,
           sf::Color bg = Theme::ACCENT, sf::Color bgH = Theme::ACCENT_HOVER,
           sf::Color bgP = Theme::ACCENT_DARK, unsigned int fontSize = 16)
        : label(font, text, fontSize), bgIdle(bg), bgHover(bgH), bgPressed(bgP)
    {
        box.setSize(size); box.setPosition(position); box.setFillColor(bgIdle);
        label.setFillColor(Theme::TEXT_PRIMARY);
        sf::FloatRect tb = label.getLocalBounds();
        label.setOrigin({tb.position.x + tb.size.x/2.f, tb.position.y + tb.size.y/2.f});
        label.setPosition({position.x + size.x/2.f, position.y + size.y/2.f});
    }
    void update(sf::Vector2f mouse, bool mouseDown) {
        hovered = box.getGlobalBounds().contains(mouse);
        pressed = hovered && mouseDown;
        if (pressed)      box.setFillColor(bgPressed);
        else if (hovered) box.setFillColor(bgHover);
        else              box.setFillColor(bgIdle);
    }
    bool wasClicked(sf::Vector2f mouse, bool mouseReleased) {
        return mouseReleased && box.getGlobalBounds().contains(mouse);
    }
    void draw(sf::RenderWindow& w) { w.draw(box); w.draw(label); }
};

// =============================================================================
//  UI: TextField
// =============================================================================
class TextField {
public:
    sf::RectangleShape box;
    sf::Text display; sf::Text placeholder;
    std::string value;
    bool focused = false; bool password = false;
    bool numeric = false; bool decimal = false;
    size_t maxLen = 200;
    sf::Vector2f position; sf::Vector2f size;

    TextField(sf::Vector2f pos, sf::Vector2f sz, const std::string& ph, bool isPw = false)
        : display(font, "", 16), placeholder(font, ph, 16),
          position(pos), size(sz), password(isPw)
    {
        box.setSize(size); box.setPosition(position);
        box.setFillColor(Theme::BG_INPUT);
        box.setOutlineThickness(2.f); box.setOutlineColor(Theme::BORDER);
        display.setFillColor(Theme::TEXT_PRIMARY);
        display.setPosition({position.x + 12.f, position.y + 10.f});
        placeholder.setFillColor(Theme::TEXT_MUTED);
        placeholder.setPosition({position.x + 12.f, position.y + 10.f});
    }
    void handleClick(sf::Vector2f mouse, bool mouseReleased) {
        if (!mouseReleased) return;
        focused = box.getGlobalBounds().contains(mouse);
        box.setOutlineColor(focused ? Theme::BORDER_FOCUS : Theme::BORDER);
    }
    void handleTextInput(char32_t unicode) {
        if (!focused) return;
        if (unicode == 8) { if (!value.empty()) value.pop_back(); }
        else if (unicode >= 32 && unicode < 127) {
            if (value.size() >= maxLen) return;
            char c = static_cast<char>(unicode);
            if (numeric && !std::isdigit((unsigned char)c)) {
                if (!(decimal && (c == '.' || c == '-'))) return;
            }
            value += c;
        }
        refresh();
    }
    void refresh() { display.setString(password ? std::string(value.size(), '*') : value); }
    void clear() { value.clear(); refresh(); }
    void setText(const std::string& s) { value = s; refresh(); }
    void draw(sf::RenderWindow& w) {
        w.draw(box);
        if (value.empty() && !focused) w.draw(placeholder); else w.draw(display);
    }
};

// =============================================================================
//  Login
// =============================================================================
TextField loginIdField   ({490, 280}, {300, 44}, "Employee ID");
TextField loginPassField ({490, 350}, {300, 44}, "Password", true);
Button    loginButton    ("LOGIN", {490, 430}, {300, 48},
                          Theme::ACCENT, Theme::ACCENT_HOVER, Theme::ACCENT_DARK, 18);
std::string loginMessage;
sf::Color   loginMessageColor = Theme::DANGER;
const std::string CRED_ID   = "123";
const std::string CRED_PASS = "pass123";

void attemptLogin() {
    if (loginIdField.value == CRED_ID && loginPassField.value == CRED_PASS) {
        loginMessage.clear();
        currentScreen = Screen::MainMenu;
        loginPassField.clear();
    } else {
        loginMessage = "Invalid Employee ID or Password";
        loginMessageColor = Theme::DANGER;
    }
}
void drawLoginScreen(sf::RenderWindow& window) {
    sf::RectangleShape logoCircle(sf::Vector2f(80.f, 80.f));
    logoCircle.setFillColor(Theme::ACCENT); logoCircle.setPosition({600, 100});
    sf::RectangleShape crossH(sf::Vector2f(40.f, 10.f));
    crossH.setFillColor(Theme::TEXT_PRIMARY); crossH.setPosition({620, 135});
    sf::RectangleShape crossV(sf::Vector2f(10.f, 40.f));
    crossV.setFillColor(Theme::TEXT_PRIMARY); crossV.setPosition({635, 120});
    window.draw(logoCircle); window.draw(crossH); window.draw(crossV);

    sf::Text title(font, "Hospital Management System", 32);
    title.setFillColor(Theme::TEXT_PRIMARY);
    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin({tb.position.x + tb.size.x/2.f, 0.f});
    title.setPosition({640, 210}); window.draw(title);
    sf::Text subtitle(font, "Sign in to continue", 16);
    subtitle.setFillColor(Theme::TEXT_SECONDARY);
    sf::FloatRect sb = subtitle.getLocalBounds();
    subtitle.setOrigin({sb.position.x + sb.size.x/2.f, 0.f});
    subtitle.setPosition({640, 248}); window.draw(subtitle);

    loginIdField.draw(window); loginPassField.draw(window); loginButton.draw(window);

    if (!loginMessage.empty()) {
        sf::Text msg(font, loginMessage, 14);
        msg.setFillColor(loginMessageColor);
        sf::FloatRect mb = msg.getLocalBounds();
        msg.setOrigin({mb.position.x + mb.size.x/2.f, 0.f});
        msg.setPosition({640, 496}); window.draw(msg);
    }
    sf::Text hint(font, "Hint: 123 / pass123", 12);
    hint.setFillColor(Theme::TEXT_MUTED);
    sf::FloatRect hb = hint.getLocalBounds();
    hint.setOrigin({hb.position.x + hb.size.x/2.f, 0.f});
    hint.setPosition({640, 680}); window.draw(hint);
}

// =============================================================================
//  Top bar
// =============================================================================
void drawTopBar(sf::RenderWindow& window) {
    sf::RectangleShape topBar(sf::Vector2f(1280.f, 70.f));
    topBar.setFillColor(Theme::BG_PANEL); topBar.setPosition({0, 0});
    window.draw(topBar);
    sf::RectangleShape logo(sf::Vector2f(36.f, 36.f));
    logo.setFillColor(Theme::ACCENT); logo.setPosition({24, 17});
    window.draw(logo);
    sf::RectangleShape miniCrossH(sf::Vector2f(18.f, 5.f));
    miniCrossH.setFillColor(Theme::TEXT_PRIMARY); miniCrossH.setPosition({33, 32});
    window.draw(miniCrossH);
    sf::RectangleShape miniCrossV(sf::Vector2f(5.f, 18.f));
    miniCrossV.setFillColor(Theme::TEXT_PRIMARY); miniCrossV.setPosition({39, 26});
    window.draw(miniCrossV);
    sf::Text brand(font, "Hospital Management System", 18);
    brand.setFillColor(Theme::TEXT_PRIMARY); brand.setPosition({76, 22});
    window.draw(brand);
    sf::Text welcome(font, "Welcome back, Admin", 14);
    welcome.setFillColor(Theme::TEXT_SECONDARY);
    sf::FloatRect wb = welcome.getLocalBounds();
    welcome.setOrigin({wb.position.x + wb.size.x, 0.f});
    welcome.setPosition({1256, 28}); window.draw(welcome);
}

// =============================================================================
//  Main Menu
// =============================================================================
struct MenuItem {
    std::string title; std::string description;
    Screen target;
    sf::Vector2f position; sf::Vector2f size;
    bool hovered = false;
};
std::vector<MenuItem> menuItems = {
    {"Patients",     "Manage patient records",     Screen::PatientList,     {80,  200}, {360, 130}},
    {"Doctors",      "Manage doctor profiles",     Screen::DoctorList,      {460, 200}, {360, 130}},
    {"Appointments", "Schedule and view bookings", Screen::AppointmentList, {840, 200}, {360, 130}},
    {"Treatments",   "Treatments and billing",     Screen::TreatmentList,   {80,  360}, {360, 130}},
    {"Search",       "Search and reports",         Screen::SearchMenu,      {460, 360}, {360, 130}},
    {"Logout",       "Return to login screen",     Screen::Login,           {840, 360}, {360, 130}},
};

void drawMainMenu(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text pageTitle(font, "Dashboard", 28);
    pageTitle.setFillColor(Theme::TEXT_PRIMARY); pageTitle.setPosition({80, 110});
    window.draw(pageTitle);
    sf::Text pageSub(font, "Choose a module to get started", 14);
    pageSub.setFillColor(Theme::TEXT_SECONDARY); pageSub.setPosition({80, 150});
    window.draw(pageSub);

    for (auto& item : menuItems) {
        sf::FloatRect cb({item.position.x, item.position.y}, {item.size.x, item.size.y});
        item.hovered = cb.contains(mouse);
        sf::RectangleShape card(item.size); card.setPosition(item.position);
        card.setFillColor(item.hovered ? Theme::BG_PANEL_HOVER : Theme::BG_PANEL);
        card.setOutlineThickness(item.hovered ? 2.f : 1.f);
        card.setOutlineColor(item.hovered ? Theme::ACCENT : Theme::BORDER);
        window.draw(card);
        sf::RectangleShape stripe(sf::Vector2f(4.f, item.size.y));
        stripe.setPosition(item.position); stripe.setFillColor(Theme::ACCENT);
        window.draw(stripe);
        sf::Text cardTitle(font, item.title, 22);
        cardTitle.setFillColor(Theme::TEXT_PRIMARY);
        cardTitle.setPosition({item.position.x + 24, item.position.y + 28});
        window.draw(cardTitle);
        sf::Text cardDesc(font, item.description, 14);
        cardDesc.setFillColor(Theme::TEXT_SECONDARY);
        cardDesc.setPosition({item.position.x + 24, item.position.y + 68});
        window.draw(cardDesc);
        sf::Text hint(font, item.hovered ? "Click to open  ->" : "->", 14);
        hint.setFillColor(item.hovered ? Theme::ACCENT_HOVER : Theme::TEXT_MUTED);
        hint.setPosition({item.position.x + 24, item.position.y + 95});
        window.draw(hint);
    }

    std::stringstream stats;
    stats << "Database: " << patients.size() << " patients - "
          << doctors.size() << " doctors - "
          << appointments.size() << " appointments - "
          << treatments.size() << " treatments";
    sf::Text footer(font, stats.str(), 12);
    footer.setFillColor(Theme::TEXT_MUTED);
    footer.setPosition({80, 680}); window.draw(footer);
}

// =============================================================================
//  Shared list constants & helpers
// =============================================================================
const float ROW_HEIGHT = 44.f;
const float TABLE_TOP = 200.f;
const float TABLE_BOTTOM = 640.f;
const float TABLE_LEFT = 80.f;
const float TABLE_WIDTH = 1120.f;

void drawListPanel(sf::RenderWindow& window, const char* const* colNames,
                   const float* colX, int colCount) {
    sf::RectangleShape panel({TABLE_WIDTH, TABLE_BOTTOM - TABLE_TOP});
    panel.setPosition({TABLE_LEFT, TABLE_TOP});
    panel.setFillColor(Theme::BG_PANEL);
    panel.setOutlineThickness(1.f); panel.setOutlineColor(Theme::BORDER);
    window.draw(panel);
    sf::RectangleShape header({TABLE_WIDTH, 44.f});
    header.setPosition({TABLE_LEFT, TABLE_TOP});
    header.setFillColor(Theme::BG_PANEL_HOVER); window.draw(header);
    for (int i = 0; i < colCount; ++i) {
        sf::Text h(font, colNames[i], 13);
        h.setFillColor(Theme::TEXT_SECONDARY);
        h.setPosition({colX[i], TABLE_TOP + 14.f});
        window.draw(h);
    }
}
void drawDisabledActionButtons(sf::RenderWindow& window) {
    sf::RectangleShape eb({120.f, 38.f}); eb.setPosition({1000, 660});
    eb.setFillColor(Theme::BG_PANEL_HOVER); window.draw(eb);
    sf::Text et(font, "Edit", 14); et.setFillColor(Theme::TEXT_MUTED);
    sf::FloatRect ebr = et.getLocalBounds();
    et.setOrigin({ebr.position.x + ebr.size.x/2.f, ebr.position.y + ebr.size.y/2.f});
    et.setPosition({1060, 679}); window.draw(et);
    sf::RectangleShape db({120.f, 38.f}); db.setPosition({1130, 660});
    db.setFillColor(Theme::BG_PANEL_HOVER); window.draw(db);
    sf::Text dt(font, "Delete", 14); dt.setFillColor(Theme::TEXT_MUTED);
    sf::FloatRect dbr = dt.getLocalBounds();
    dt.setOrigin({dbr.position.x + dbr.size.x/2.f, dbr.position.y + dbr.size.y/2.f});
    dt.setPosition({1190, 679}); window.draw(dt);
    sf::Text hint(font, "Click a row to select", 14);
    hint.setFillColor(Theme::TEXT_MUTED);
    hint.setPosition({80, 670}); window.draw(hint);
}
void drawFormLabel(sf::RenderWindow& w, const std::string& label, float y) {
    sf::Text t(font, label, 13);
    t.setFillColor(Theme::TEXT_SECONDARY);
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin({b.position.x + b.size.x, 0.f});
    t.setPosition({470.f, y + 12.f}); w.draw(t);
}

// =============================================================================
//  Patient List
// =============================================================================
Button btnBackP("<- Back", {1130, 90}, {120, 38},
                Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
Button btnAddP ("+ Add Patient", {980, 90}, {140, 38});
Button btnEditP("Edit", {1000, 660}, {120, 38},
                Theme::WARNING, sf::Color(253,224,71), Theme::WARNING_DARK);
Button btnDelP ("Delete", {1130, 660}, {120, 38},
                Theme::DANGER, Theme::DANGER_HOVER, Theme::DANGER_DARK);
float patientScrollY = 0.f;

void drawPatientList(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text pageTitle(font, "Patient Management", 28);
    pageTitle.setFillColor(Theme::TEXT_PRIMARY); pageTitle.setPosition({80, 100});
    window.draw(pageTitle);
    sf::Text pageSub(font, std::to_string(patients.size()) + " patient" +
        (patients.size() == 1 ? "" : "s") + " on record", 14);
    pageSub.setFillColor(Theme::TEXT_SECONDARY); pageSub.setPosition({80, 140});
    window.draw(pageSub);
    btnBackP.draw(window); btnAddP.draw(window);
    static const float colX[] = {100, 180, 470, 560, 670, 850};
    static const char* colNames[] = {"ID", "Name", "Age", "Gender", "Contact", "Balance"};
    drawListPanel(window, colNames, colX, 6);

    if (patients.empty()) {
        sf::Text empty(font, "No patients yet - click Add Patient to start", 16);
        empty.setFillColor(Theme::TEXT_MUTED);
        sf::FloatRect eb = empty.getLocalBounds();
        empty.setOrigin({eb.position.x + eb.size.x/2.f, 0.f});
        empty.setPosition({TABLE_LEFT + TABLE_WIDTH/2.f, TABLE_TOP + 180.f});
        window.draw(empty);
    }
    const float ROWS_TOP = TABLE_TOP + 44.f;
    for (size_t i = 0; i < patients.size(); ++i) {
        float y = ROWS_TOP + i * ROW_HEIGHT - patientScrollY;
        if (y + ROW_HEIGHT < ROWS_TOP || y > TABLE_BOTTOM) continue;
        sf::RectangleShape row({TABLE_WIDTH, ROW_HEIGHT}); row.setPosition({TABLE_LEFT, y});
        sf::FloatRect rb({TABLE_LEFT, y}, {TABLE_WIDTH, ROW_HEIGHT});
        bool hovered = rb.contains(mouse) && y >= ROWS_TOP && y + ROW_HEIGHT <= TABLE_BOTTOM;
        bool selected = ((int)i == selectedPatientIndex);
        if (selected)      row.setFillColor(Theme::BG_ROW_SELECTED);
        else if (hovered)  row.setFillColor(Theme::BG_PANEL_HOVER);
        else               row.setFillColor(i % 2 == 0 ? Theme::BG_PANEL : Theme::BG_ROW_ALT);
        window.draw(row);
        if (selected) {
            sf::RectangleShape stripe({4.f, ROW_HEIGHT}); stripe.setPosition({TABLE_LEFT, y});
            stripe.setFillColor(Theme::ACCENT); window.draw(stripe);
        }
        const Patient& p = patients[i];
        sf::Text c1(font, std::to_string(p.patientid), 14); c1.setFillColor(Theme::TEXT_PRIMARY); c1.setPosition({colX[0], y+12}); window.draw(c1);
        sf::Text c2(font, p.name, 14); c2.setFillColor(Theme::TEXT_PRIMARY); c2.setPosition({colX[1], y+12}); window.draw(c2);
        sf::Text c3(font, std::to_string(p.age), 14); c3.setFillColor(Theme::TEXT_SECONDARY); c3.setPosition({colX[2], y+12}); window.draw(c3);
        sf::Text c4(font, p.gender, 14); c4.setFillColor(Theme::TEXT_SECONDARY); c4.setPosition({colX[3], y+12}); window.draw(c4);
        sf::Text c5(font, p.contact, 14); c5.setFillColor(Theme::TEXT_SECONDARY); c5.setPosition({colX[4], y+12}); window.draw(c5);
        sf::Text c6(font, moneyStr(p.balance), 14); c6.setFillColor(Theme::TEXT_PRIMARY); c6.setPosition({colX[5], y+12}); window.draw(c6);
    }
    bool hasSel = (selectedPatientIndex >= 0 && selectedPatientIndex < (int)patients.size());
    if (hasSel) {
        btnEditP.draw(window); btnDelP.draw(window);
        const Patient& p = patients[selectedPatientIndex];
        sf::Text sel(font, "Selected: " + p.name + " (#" + std::to_string(p.patientid) + ")", 14);
        sel.setFillColor(Theme::ACCENT_HOVER); sel.setPosition({80, 670}); window.draw(sel);
    } else drawDisabledActionButtons(window);
}
void handlePatientListClick(sf::Vector2f mouse) {
    const float ROWS_TOP = TABLE_TOP + 44.f;
    if (mouse.y < ROWS_TOP || mouse.y > TABLE_BOTTOM) return;
    if (mouse.x < TABLE_LEFT || mouse.x > TABLE_LEFT + TABLE_WIDTH) return;
    int idx = static_cast<int>((mouse.y - ROWS_TOP + patientScrollY) / ROW_HEIGHT);
    if (idx < 0 || idx >= (int)patients.size()) { selectedPatientIndex = -1; return; }
    selectedPatientIndex = (selectedPatientIndex == idx) ? -1 : idx;
}

// =============================================================================
//  Patient Form
// =============================================================================
TextField fId      ({490, 200}, {350, 40}, "Patient ID (digits only)");
TextField fName    ({490, 260}, {350, 40}, "Full name");
TextField fAge     ({490, 320}, {350, 40}, "Age (0 - 120)");
TextField fGender  ({490, 380}, {350, 40}, "Gender (Male/Female/M/F)");
TextField fContact ({490, 440}, {350, 40}, "Contact (11 digits)");
TextField fBalance ({490, 500}, {350, 40}, "Balance");
Button btnSave  ("Save", {490, 580}, {165, 44});
Button btnCancel("Cancel", {675, 580}, {165, 44},
                 Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
std::string formError;
bool isEditMode = false;
int  editingId = -1;
void resetPatientForm() {
    fId.clear(); fName.clear(); fAge.clear();
    fGender.clear(); fContact.clear(); fBalance.clear();
    fId.numeric = true; fId.maxLen = 9;
    fName.maxLen = 50; fAge.numeric = true; fAge.maxLen = 3;
    fGender.maxLen = 10; fContact.numeric = true; fContact.maxLen = 11;
    fBalance.numeric = true; fBalance.decimal = true; fBalance.maxLen = 12;
    formError.clear();
}
void prefillPatientForm() {
    if (selectedPatientIndex < 0 || selectedPatientIndex >= (int)patients.size()) return;
    const Patient& p = patients[selectedPatientIndex];
    fId.setText(std::to_string(p.patientid));
    fName.setText(p.name); fAge.setText(std::to_string(p.age));
    fGender.setText(p.gender); fContact.setText(p.contact);
    fBalance.setText(moneyStr(p.balance));
    editingId = p.patientid;
}
bool validatePatient(Patient& out) {
    if (fId.value.empty()) { formError = "Patient ID is required"; return false; }
    int id; try { id = std::stoi(fId.value); } catch (...) { formError = "Patient ID must be numeric"; return false; }
    if (id <= 0) { formError = "Patient ID must be positive"; return false; }
    if (!isEditMode || id != editingId)
        for (const auto& p : patients) if (p.patientid == id) { formError = "A patient with this ID already exists"; return false; }
    if (fName.value.empty()) { formError = "Name cannot be empty"; return false; }
    int age = 0;
    if (!fAge.value.empty()) { try { age = std::stoi(fAge.value); } catch (...) { formError = "Age must be a number"; return false; } }
    if (age < 0 || age > 120) { formError = "Age must be between 0 and 120"; return false; }
    if (!validContact(fContact.value)) { formError = "Contact must be exactly 11 digits"; return false; }
    double balance = 0.0;
    if (!fBalance.value.empty()) { try { balance = std::stod(fBalance.value); } catch (...) { formError = "Balance must be a number"; return false; } }
    out.patientid = id; out.name = fName.value; out.age = age;
    out.gender = fixGender(fGender.value); out.contact = fContact.value; out.balance = balance;
    return true;
}
void drawPatientForm(sf::RenderWindow& window) {
    drawTopBar(window);
    sf::Text title(font, isEditMode ? "Edit Patient" : "Add Patient", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    sf::Text sub(font, isEditMode ? "Update the fields below" : "Fill in the details for the new patient", 14);
    sub.setFillColor(Theme::TEXT_SECONDARY); sub.setPosition({80, 140}); window.draw(sub);
    sf::RectangleShape panel({900.f, 460.f}); panel.setPosition({190.f, 180.f});
    panel.setFillColor(Theme::BG_PANEL); panel.setOutlineThickness(1.f);
    panel.setOutlineColor(Theme::BORDER); window.draw(panel);
    drawFormLabel(window, "Patient ID:", 200);
    drawFormLabel(window, "Name:",        260);
    drawFormLabel(window, "Age:",         320);
    drawFormLabel(window, "Gender:",      380);
    drawFormLabel(window, "Contact:",     440);
    drawFormLabel(window, "Balance:",     500);
    fId.draw(window); fName.draw(window); fAge.draw(window);
    fGender.draw(window); fContact.draw(window); fBalance.draw(window);
    if (!formError.empty()) {
        sf::Text err(font, formError, 13);
        err.setFillColor(Theme::DANGER); err.setPosition({490, 545}); window.draw(err);
    }
    btnSave.draw(window); btnCancel.draw(window);
}

// =============================================================================
//  Doctor List
// =============================================================================
Button btnBackD("<- Back", {1130, 90}, {120, 38},
                Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
Button btnAddD ("+ Add Doctor", {980, 90}, {140, 38});
Button btnEditD("Edit", {1000, 660}, {120, 38},
                Theme::WARNING, sf::Color(253,224,71), Theme::WARNING_DARK);
Button btnDelD ("Delete", {1130, 660}, {120, 38},
                Theme::DANGER, Theme::DANGER_HOVER, Theme::DANGER_DARK);
float doctorScrollY = 0.f;

void drawDoctorList(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text pageTitle(font, "Doctor Management", 28);
    pageTitle.setFillColor(Theme::TEXT_PRIMARY); pageTitle.setPosition({80, 100});
    window.draw(pageTitle);
    sf::Text pageSub(font, std::to_string(doctors.size()) + " doctor" +
        (doctors.size() == 1 ? "" : "s") + " on record", 14);
    pageSub.setFillColor(Theme::TEXT_SECONDARY); pageSub.setPosition({80, 140});
    window.draw(pageSub);
    btnBackD.draw(window); btnAddD.draw(window);
    static const float colX[] = {100, 220, 580, 950};
    static const char* colNames[] = {"ID", "Name", "Speciality", "Experience"};
    drawListPanel(window, colNames, colX, 4);
    if (doctors.empty()) {
        sf::Text empty(font, "No doctors yet - click Add Doctor to start", 16);
        empty.setFillColor(Theme::TEXT_MUTED);
        sf::FloatRect eb = empty.getLocalBounds();
        empty.setOrigin({eb.position.x + eb.size.x/2.f, 0.f});
        empty.setPosition({TABLE_LEFT + TABLE_WIDTH/2.f, TABLE_TOP + 180.f}); window.draw(empty);
    }
    const float ROWS_TOP = TABLE_TOP + 44.f;
    for (size_t i = 0; i < doctors.size(); ++i) {
        float y = ROWS_TOP + i * ROW_HEIGHT - doctorScrollY;
        if (y + ROW_HEIGHT < ROWS_TOP || y > TABLE_BOTTOM) continue;
        sf::RectangleShape row({TABLE_WIDTH, ROW_HEIGHT}); row.setPosition({TABLE_LEFT, y});
        sf::FloatRect rb({TABLE_LEFT, y}, {TABLE_WIDTH, ROW_HEIGHT});
        bool hovered = rb.contains(mouse) && y >= ROWS_TOP && y + ROW_HEIGHT <= TABLE_BOTTOM;
        bool selected = ((int)i == selectedDoctorIndex);
        if (selected)      row.setFillColor(Theme::BG_ROW_SELECTED);
        else if (hovered)  row.setFillColor(Theme::BG_PANEL_HOVER);
        else               row.setFillColor(i % 2 == 0 ? Theme::BG_PANEL : Theme::BG_ROW_ALT);
        window.draw(row);
        if (selected) {
            sf::RectangleShape stripe({4.f, ROW_HEIGHT}); stripe.setPosition({TABLE_LEFT, y});
            stripe.setFillColor(Theme::ACCENT); window.draw(stripe);
        }
        const Doctor& d = doctors[i];
        sf::Text c1(font, std::to_string(d.docid), 14); c1.setFillColor(Theme::TEXT_PRIMARY); c1.setPosition({colX[0], y+12}); window.draw(c1);
        sf::Text c2(font, d.name, 14); c2.setFillColor(Theme::TEXT_PRIMARY); c2.setPosition({colX[1], y+12}); window.draw(c2);
        sf::Text c3(font, d.speciality, 14); c3.setFillColor(Theme::TEXT_SECONDARY); c3.setPosition({colX[2], y+12}); window.draw(c3);
        sf::Text c4(font, std::to_string(d.experience) + " yrs", 14); c4.setFillColor(Theme::TEXT_SECONDARY); c4.setPosition({colX[3], y+12}); window.draw(c4);
    }
    bool hasSel = (selectedDoctorIndex >= 0 && selectedDoctorIndex < (int)doctors.size());
    if (hasSel) {
        btnEditD.draw(window); btnDelD.draw(window);
        const Doctor& d = doctors[selectedDoctorIndex];
        sf::Text sel(font, "Selected: Dr. " + d.name + " (#" + std::to_string(d.docid) + ")", 14);
        sel.setFillColor(Theme::ACCENT_HOVER); sel.setPosition({80, 670}); window.draw(sel);
    } else drawDisabledActionButtons(window);
}
void handleDoctorListClick(sf::Vector2f mouse) {
    const float ROWS_TOP = TABLE_TOP + 44.f;
    if (mouse.y < ROWS_TOP || mouse.y > TABLE_BOTTOM) return;
    if (mouse.x < TABLE_LEFT || mouse.x > TABLE_LEFT + TABLE_WIDTH) return;
    int idx = static_cast<int>((mouse.y - ROWS_TOP + doctorScrollY) / ROW_HEIGHT);
    if (idx < 0 || idx >= (int)doctors.size()) { selectedDoctorIndex = -1; return; }
    selectedDoctorIndex = (selectedDoctorIndex == idx) ? -1 : idx;
}

// =============================================================================
//  Doctor Form
// =============================================================================
TextField fdId   ({490, 220}, {350, 40}, "Doctor ID (digits only)");
TextField fdName ({490, 290}, {350, 40}, "Full name");
TextField fdSpec ({490, 360}, {350, 40}, "Speciality");
TextField fdExp  ({490, 430}, {350, 40}, "Experience (years)");
Button btnSaveD  ("Save", {490, 540}, {165, 44});
Button btnCancelD("Cancel", {675, 540}, {165, 44},
                  Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
std::string doctorFormError;
bool docEditMode = false;
int  docEditingId = -1;
void resetDoctorForm() {
    fdId.clear(); fdName.clear(); fdSpec.clear(); fdExp.clear();
    fdId.numeric = true; fdId.maxLen = 9;
    fdName.maxLen = 50; fdSpec.maxLen = 50;
    fdExp.numeric = true; fdExp.maxLen = 3;
    doctorFormError.clear();
}
void prefillDoctorForm() {
    if (selectedDoctorIndex < 0 || selectedDoctorIndex >= (int)doctors.size()) return;
    const Doctor& d = doctors[selectedDoctorIndex];
    fdId.setText(std::to_string(d.docid)); fdName.setText(d.name);
    fdSpec.setText(d.speciality); fdExp.setText(std::to_string(d.experience));
    docEditingId = d.docid;
}
bool validateDoctor(Doctor& out) {
    if (fdId.value.empty()) { doctorFormError = "Doctor ID is required"; return false; }
    int id; try { id = std::stoi(fdId.value); } catch (...) { doctorFormError = "Doctor ID must be numeric"; return false; }
    if (id <= 0) { doctorFormError = "Doctor ID must be positive"; return false; }
    if (!docEditMode || id != docEditingId)
        for (const auto& d : doctors) if (d.docid == id) { doctorFormError = "A doctor with this ID already exists"; return false; }
    if (fdName.value.empty()) { doctorFormError = "Name cannot be empty"; return false; }
    if (fdSpec.value.empty()) { doctorFormError = "Speciality cannot be empty"; return false; }
    int exp = 0;
    if (!fdExp.value.empty()) { try { exp = std::stoi(fdExp.value); } catch (...) { doctorFormError = "Experience must be a number"; return false; } }
    if (exp < 0 || exp > 70) { doctorFormError = "Experience must be between 0 and 70"; return false; }
    out.docid = id; out.name = fdName.value; out.speciality = fdSpec.value; out.experience = exp;
    return true;
}
void drawDoctorForm(sf::RenderWindow& window) {
    drawTopBar(window);
    sf::Text title(font, docEditMode ? "Edit Doctor" : "Add Doctor", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    sf::Text sub(font, docEditMode ? "Update the doctor's details" : "Fill in the details for the new doctor", 14);
    sub.setFillColor(Theme::TEXT_SECONDARY); sub.setPosition({80, 140}); window.draw(sub);
    sf::RectangleShape panel({900.f, 420.f}); panel.setPosition({190.f, 200.f});
    panel.setFillColor(Theme::BG_PANEL); panel.setOutlineThickness(1.f);
    panel.setOutlineColor(Theme::BORDER); window.draw(panel);
    drawFormLabel(window, "Doctor ID:", 220);
    drawFormLabel(window, "Name:",      290);
    drawFormLabel(window, "Speciality:", 360);
    drawFormLabel(window, "Experience:", 430);
    fdId.draw(window); fdName.draw(window); fdSpec.draw(window); fdExp.draw(window);
    if (!doctorFormError.empty()) {
        sf::Text err(font, doctorFormError, 13);
        err.setFillColor(Theme::DANGER); err.setPosition({490, 500}); window.draw(err);
    }
    btnSaveD.draw(window); btnCancelD.draw(window);
}

// =============================================================================
//  Appointment List + Form
// =============================================================================
Button btnBackA("<- Back", {1130, 90}, {120, 38},
                Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
Button btnAddA ("+ New Appointment", {950, 90}, {170, 38});
Button btnEditA("Edit", {1000, 660}, {120, 38},
                Theme::WARNING, sf::Color(253,224,71), Theme::WARNING_DARK);
Button btnDelA ("Delete", {1130, 660}, {120, 38},
                Theme::DANGER, Theme::DANGER_HOVER, Theme::DANGER_DARK);
float apptScrollY = 0.f;

void drawApptList(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text pageTitle(font, "Appointments", 28);
    pageTitle.setFillColor(Theme::TEXT_PRIMARY); pageTitle.setPosition({80, 100}); window.draw(pageTitle);
    sf::Text pageSub(font, std::to_string(appointments.size()) + " appointment" +
        (appointments.size() == 1 ? "" : "s") + " scheduled", 14);
    pageSub.setFillColor(Theme::TEXT_SECONDARY); pageSub.setPosition({80, 140}); window.draw(pageSub);
    btnBackA.draw(window); btnAddA.draw(window);
    static const float colX[] = {100, 400, 750, 950};
    static const char* colNames[] = {"Patient", "Doctor", "Date", "Time"};
    drawListPanel(window, colNames, colX, 4);
    if (appointments.empty()) {
        sf::Text empty(font, "No appointments yet - click New Appointment to schedule one", 16);
        empty.setFillColor(Theme::TEXT_MUTED);
        sf::FloatRect eb = empty.getLocalBounds();
        empty.setOrigin({eb.position.x + eb.size.x/2.f, 0.f});
        empty.setPosition({TABLE_LEFT + TABLE_WIDTH/2.f, TABLE_TOP + 180.f}); window.draw(empty);
    }
    const float ROWS_TOP = TABLE_TOP + 44.f;
    for (size_t i = 0; i < appointments.size(); ++i) {
        float y = ROWS_TOP + i * ROW_HEIGHT - apptScrollY;
        if (y + ROW_HEIGHT < ROWS_TOP || y > TABLE_BOTTOM) continue;
        sf::RectangleShape row({TABLE_WIDTH, ROW_HEIGHT}); row.setPosition({TABLE_LEFT, y});
        sf::FloatRect rb({TABLE_LEFT, y}, {TABLE_WIDTH, ROW_HEIGHT});
        bool hovered = rb.contains(mouse) && y >= ROWS_TOP && y + ROW_HEIGHT <= TABLE_BOTTOM;
        bool selected = ((int)i == selectedApptIndex);
        if (selected)      row.setFillColor(Theme::BG_ROW_SELECTED);
        else if (hovered)  row.setFillColor(Theme::BG_PANEL_HOVER);
        else               row.setFillColor(i % 2 == 0 ? Theme::BG_PANEL : Theme::BG_ROW_ALT);
        window.draw(row);
        if (selected) {
            sf::RectangleShape stripe({4.f, ROW_HEIGHT}); stripe.setPosition({TABLE_LEFT, y});
            stripe.setFillColor(Theme::ACCENT); window.draw(stripe);
        }
        const Appointment& a = appointments[i];
        sf::Text c1(font, getPatientName(a.patientid) + " (#" + std::to_string(a.patientid) + ")", 14);
        c1.setFillColor(Theme::TEXT_PRIMARY); c1.setPosition({colX[0], y+12}); window.draw(c1);
        sf::Text c2(font, "Dr. " + getDoctorName(a.doctorid) + " (#" + std::to_string(a.doctorid) + ")", 14);
        c2.setFillColor(Theme::TEXT_PRIMARY); c2.setPosition({colX[1], y+12}); window.draw(c2);
        sf::Text c3(font, a.date, 14); c3.setFillColor(Theme::TEXT_SECONDARY); c3.setPosition({colX[2], y+12}); window.draw(c3);
        sf::Text c4(font, a.time, 14); c4.setFillColor(Theme::TEXT_SECONDARY); c4.setPosition({colX[3], y+12}); window.draw(c4);
    }
    bool hasSel = (selectedApptIndex >= 0 && selectedApptIndex < (int)appointments.size());
    if (hasSel) {
        btnEditA.draw(window); btnDelA.draw(window);
        const Appointment& a = appointments[selectedApptIndex];
        sf::Text sel(font, "Selected: " + getPatientName(a.patientid) + " with Dr. " +
                            getDoctorName(a.doctorid) + " on " + a.date, 14);
        sel.setFillColor(Theme::ACCENT_HOVER); sel.setPosition({80, 670}); window.draw(sel);
    } else drawDisabledActionButtons(window);
}
void handleApptListClick(sf::Vector2f mouse) {
    const float ROWS_TOP = TABLE_TOP + 44.f;
    if (mouse.y < ROWS_TOP || mouse.y > TABLE_BOTTOM) return;
    if (mouse.x < TABLE_LEFT || mouse.x > TABLE_LEFT + TABLE_WIDTH) return;
    int idx = static_cast<int>((mouse.y - ROWS_TOP + apptScrollY) / ROW_HEIGHT);
    if (idx < 0 || idx >= (int)appointments.size()) { selectedApptIndex = -1; return; }
    selectedApptIndex = (selectedApptIndex == idx) ? -1 : idx;
}

TextField faPid ({490, 220}, {350, 40}, "Patient ID");
TextField faDid ({490, 290}, {350, 40}, "Doctor ID");
TextField faDate({490, 360}, {350, 40}, "Date (MM-DD-YYYY)");
TextField faTime({490, 430}, {350, 40}, "Time (e.g. 09:30 AM or 14:30)");
Button btnSaveA ("Save", {490, 540}, {165, 44});
Button btnCancelA("Cancel", {675, 540}, {165, 44},
                  Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
std::string apptFormError;
bool apptEditMode = false;
int  apptEditingIndex = -1;
void resetApptForm() {
    faPid.clear(); faDid.clear(); faDate.clear(); faTime.clear();
    faPid.numeric = true; faPid.maxLen = 9;
    faDid.numeric = true; faDid.maxLen = 9;
    faDate.maxLen = 10; faTime.maxLen = 10;
    apptFormError.clear();
}
void prefillApptForm() {
    if (selectedApptIndex < 0 || selectedApptIndex >= (int)appointments.size()) return;
    const Appointment& a = appointments[selectedApptIndex];
    faPid.setText(std::to_string(a.patientid)); faDid.setText(std::to_string(a.doctorid));
    faDate.setText(a.date); faTime.setText(a.time);
    apptEditingIndex = selectedApptIndex;
}
bool validateAppointment(Appointment& out) {
    if (faPid.value.empty()) { apptFormError = "Patient ID is required"; return false; }
    int pid; try { pid = std::stoi(faPid.value); } catch (...) { apptFormError = "Patient ID must be numeric"; return false; }
    if (!patientExists(pid)) { apptFormError = "No patient with ID " + std::to_string(pid); return false; }
    if (faDid.value.empty()) { apptFormError = "Doctor ID is required"; return false; }
    int did; try { did = std::stoi(faDid.value); } catch (...) { apptFormError = "Doctor ID must be numeric"; return false; }
    if (!doctorExists(did)) { apptFormError = "No doctor with ID " + std::to_string(did); return false; }
    std::string date = fixDate(faDate.value);
    if (!validDate(date)) { apptFormError = "Date must be MM-DD-YYYY"; return false; }
    if (!validTime(faTime.value)) { apptFormError = "Time format invalid"; return false; }
    std::string ctime = fixTime(faTime.value);
    out.patientid = pid; out.doctorid = did; out.date = date; out.time = ctime;
    return true;
}
void drawApptForm(sf::RenderWindow& window) {
    drawTopBar(window);
    sf::Text title(font, apptEditMode ? "Edit Appointment" : "New Appointment", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    sf::Text sub(font, apptEditMode ? "Update appointment details" : "Schedule a new appointment", 14);
    sub.setFillColor(Theme::TEXT_SECONDARY); sub.setPosition({80, 140}); window.draw(sub);
    sf::RectangleShape panel({900.f, 420.f}); panel.setPosition({190.f, 200.f});
    panel.setFillColor(Theme::BG_PANEL); panel.setOutlineThickness(1.f);
    panel.setOutlineColor(Theme::BORDER); window.draw(panel);
    drawFormLabel(window, "Patient ID:", 220);
    drawFormLabel(window, "Doctor ID:",  290);
    drawFormLabel(window, "Date:",       360);
    drawFormLabel(window, "Time:",       430);
    faPid.draw(window); faDid.draw(window); faDate.draw(window); faTime.draw(window);
    auto showLookup = [&](const std::string& v, float y, bool isPatient) {
        if (v.empty()) return;
        try {
            int id = std::stoi(v);
            std::string name = isPatient ? getPatientName(id) : getDoctorName(id);
            bool exists = isPatient ? patientExists(id) : doctorExists(id);
            sf::Text t(font, exists ? (isPatient ? name : "Dr. " + name) : "(no match)", 12);
            t.setFillColor(exists ? Theme::ACCENT_HOVER : Theme::DANGER);
            t.setPosition({855, y + 12}); window.draw(t);
        } catch (...) {}
    };
    showLookup(faPid.value, 220, true);
    showLookup(faDid.value, 290, false);
    if (!apptFormError.empty()) {
        sf::Text err(font, apptFormError, 13);
        err.setFillColor(Theme::DANGER); err.setPosition({490, 500}); window.draw(err);
    }
    btnSaveA.draw(window); btnCancelA.draw(window);
}

// =============================================================================
//  Treatment List + Form
// =============================================================================
Button btnBackT("<- Back", {1130, 90}, {120, 38},
                Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
Button btnAddT ("+ Add Treatment", {950, 90}, {170, 38});
Button btnGoBilling("Process Billing", {770, 90}, {170, 38},
                    Theme::SUCCESS, sf::Color(74,222,128), Theme::SUCCESS_DARK);
Button btnEditT("Edit", {1000, 660}, {120, 38},
                Theme::WARNING, sf::Color(253,224,71), Theme::WARNING_DARK);
Button btnDelT ("Delete", {1130, 660}, {120, 38},
                Theme::DANGER, Theme::DANGER_HOVER, Theme::DANGER_DARK);
float treatmentScrollY = 0.f;

void drawStatusBadge(sf::RenderWindow& w, float x, float y, bool paid) {
    sf::RectangleShape pill({60.f, 22.f}); pill.setPosition({x, y});
    pill.setFillColor(paid ? Theme::SUCCESS_DARK : Theme::DANGER_DARK); w.draw(pill);
    sf::Text t(font, paid ? "Paid" : "Unpaid", 11);
    t.setFillColor(Theme::TEXT_PRIMARY);
    sf::FloatRect tb = t.getLocalBounds();
    t.setOrigin({tb.position.x + tb.size.x/2.f, tb.position.y + tb.size.y/2.f});
    t.setPosition({x + 30.f, y + 11.f}); w.draw(t);
}

void drawTreatmentList(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text pageTitle(font, "Treatments & Billing", 28);
    pageTitle.setFillColor(Theme::TEXT_PRIMARY); pageTitle.setPosition({80, 100}); window.draw(pageTitle);
    int unpaidCount = 0; double unpaidTotal = 0.0;
    for (const auto& t : treatments) if (!t.paid) { unpaidCount++; unpaidTotal += t.cost; }
    std::stringstream sub;
    sub << treatments.size() << " treatment" << (treatments.size() == 1 ? "" : "s")
        << " - " << unpaidCount << " unpaid (" << moneyStr(unpaidTotal) << " outstanding)";
    sf::Text pageSub(font, sub.str(), 14);
    pageSub.setFillColor(Theme::TEXT_SECONDARY); pageSub.setPosition({80, 140}); window.draw(pageSub);
    btnBackT.draw(window); btnAddT.draw(window); btnGoBilling.draw(window);
    static const float colX[] = {100, 320, 700, 920};
    static const char* colNames[] = {"Patient", "Description", "Cost", "Status"};
    drawListPanel(window, colNames, colX, 4);
    if (treatments.empty()) {
        sf::Text empty(font, "No treatments yet - click Add Treatment to log one", 16);
        empty.setFillColor(Theme::TEXT_MUTED);
        sf::FloatRect eb = empty.getLocalBounds();
        empty.setOrigin({eb.position.x + eb.size.x/2.f, 0.f});
        empty.setPosition({TABLE_LEFT + TABLE_WIDTH/2.f, TABLE_TOP + 180.f}); window.draw(empty);
    }
    const float ROWS_TOP = TABLE_TOP + 44.f;
    for (size_t i = 0; i < treatments.size(); ++i) {
        float y = ROWS_TOP + i * ROW_HEIGHT - treatmentScrollY;
        if (y + ROW_HEIGHT < ROWS_TOP || y > TABLE_BOTTOM) continue;
        sf::RectangleShape row({TABLE_WIDTH, ROW_HEIGHT}); row.setPosition({TABLE_LEFT, y});
        sf::FloatRect rb({TABLE_LEFT, y}, {TABLE_WIDTH, ROW_HEIGHT});
        bool hovered = rb.contains(mouse) && y >= ROWS_TOP && y + ROW_HEIGHT <= TABLE_BOTTOM;
        bool selected = ((int)i == selectedTreatmentIndex);
        if (selected)      row.setFillColor(Theme::BG_ROW_SELECTED);
        else if (hovered)  row.setFillColor(Theme::BG_PANEL_HOVER);
        else               row.setFillColor(i % 2 == 0 ? Theme::BG_PANEL : Theme::BG_ROW_ALT);
        window.draw(row);
        if (selected) {
            sf::RectangleShape stripe({4.f, ROW_HEIGHT}); stripe.setPosition({TABLE_LEFT, y});
            stripe.setFillColor(Theme::ACCENT); window.draw(stripe);
        }
        const Treatment& t = treatments[i];
        sf::Text c1(font, getPatientName(t.patientid) + " (#" + std::to_string(t.patientid) + ")", 14);
        c1.setFillColor(Theme::TEXT_PRIMARY); c1.setPosition({colX[0], y+12}); window.draw(c1);
        sf::Text c2(font, t.description, 14); c2.setFillColor(Theme::TEXT_PRIMARY); c2.setPosition({colX[1], y+12}); window.draw(c2);
        sf::Text c3(font, moneyStr(t.cost), 14); c3.setFillColor(Theme::TEXT_SECONDARY); c3.setPosition({colX[2], y+12}); window.draw(c3);
        drawStatusBadge(window, colX[3], y + 11, t.paid);
    }
    bool hasSel = (selectedTreatmentIndex >= 0 && selectedTreatmentIndex < (int)treatments.size());
    if (hasSel) {
        btnEditT.draw(window); btnDelT.draw(window);
        const Treatment& t = treatments[selectedTreatmentIndex];
        sf::Text sel(font, "Selected: " + t.description + " for " + getPatientName(t.patientid), 14);
        sel.setFillColor(Theme::ACCENT_HOVER); sel.setPosition({80, 670}); window.draw(sel);
    } else drawDisabledActionButtons(window);
}
void handleTreatmentListClick(sf::Vector2f mouse) {
    const float ROWS_TOP = TABLE_TOP + 44.f;
    if (mouse.y < ROWS_TOP || mouse.y > TABLE_BOTTOM) return;
    if (mouse.x < TABLE_LEFT || mouse.x > TABLE_LEFT + TABLE_WIDTH) return;
    int idx = static_cast<int>((mouse.y - ROWS_TOP + treatmentScrollY) / ROW_HEIGHT);
    if (idx < 0 || idx >= (int)treatments.size()) { selectedTreatmentIndex = -1; return; }
    selectedTreatmentIndex = (selectedTreatmentIndex == idx) ? -1 : idx;
}

TextField ftPid ({490, 220}, {350, 40}, "Patient ID");
TextField ftDesc({490, 290}, {350, 40}, "Description");
TextField ftCost({490, 360}, {350, 40}, "Cost");
Button btnSaveT ("Save", {490, 470}, {165, 44});
Button btnCancelT("Cancel", {675, 470}, {165, 44},
                  Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
std::string treatmentFormError;
bool treatmentEditMode = false;
int  treatmentEditingIndex = -1;
void resetTreatmentForm() {
    ftPid.clear(); ftDesc.clear(); ftCost.clear();
    ftPid.numeric = true; ftPid.maxLen = 9;
    ftDesc.maxLen = 80;
    ftCost.numeric = true; ftCost.decimal = true; ftCost.maxLen = 12;
    treatmentFormError.clear();
}
void prefillTreatmentForm() {
    if (selectedTreatmentIndex < 0 || selectedTreatmentIndex >= (int)treatments.size()) return;
    const Treatment& t = treatments[selectedTreatmentIndex];
    ftPid.setText(std::to_string(t.patientid));
    ftDesc.setText(t.description); ftCost.setText(moneyStr(t.cost));
    treatmentEditingIndex = selectedTreatmentIndex;
}
bool validateTreatment(Treatment& out, bool& wasPaid) {
    if (ftPid.value.empty()) { treatmentFormError = "Patient ID is required"; return false; }
    int pid; try { pid = std::stoi(ftPid.value); } catch (...) { treatmentFormError = "Patient ID must be numeric"; return false; }
    if (!patientExists(pid)) { treatmentFormError = "No patient with ID " + std::to_string(pid); return false; }
    if (ftDesc.value.empty()) { treatmentFormError = "Description cannot be empty"; return false; }
    double cost = 0.0;
    if (!ftCost.value.empty()) { try { cost = std::stod(ftCost.value); } catch (...) { treatmentFormError = "Cost must be a number"; return false; } }
    if (cost < 0) { treatmentFormError = "Cost cannot be negative"; return false; }
    out.patientid = pid; out.description = ftDesc.value; out.cost = cost;
    if (treatmentEditMode) { wasPaid = treatments[treatmentEditingIndex].paid; out.paid = wasPaid; }
    else                   { out.paid = false; wasPaid = false; }
    return true;
}
void drawTreatmentForm(sf::RenderWindow& window) {
    drawTopBar(window);
    sf::Text title(font, treatmentEditMode ? "Edit Treatment" : "Add Treatment", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    sf::Text sub(font, treatmentEditMode ? "Update the treatment details" : "Log a new treatment for a patient", 14);
    sub.setFillColor(Theme::TEXT_SECONDARY); sub.setPosition({80, 140}); window.draw(sub);
    sf::RectangleShape panel({900.f, 360.f}); panel.setPosition({190.f, 200.f});
    panel.setFillColor(Theme::BG_PANEL); panel.setOutlineThickness(1.f);
    panel.setOutlineColor(Theme::BORDER); window.draw(panel);
    drawFormLabel(window, "Patient ID:",  220);
    drawFormLabel(window, "Description:", 290);
    drawFormLabel(window, "Cost:",        360);
    ftPid.draw(window); ftDesc.draw(window); ftCost.draw(window);
    if (!ftPid.value.empty()) {
        try {
            int pid = std::stoi(ftPid.value);
            bool exists = patientExists(pid);
            sf::Text t(font, exists ? getPatientName(pid) : "(no match)", 12);
            t.setFillColor(exists ? Theme::ACCENT_HOVER : Theme::DANGER);
            t.setPosition({855, 232}); window.draw(t);
        } catch (...) {}
    }
    if (!treatmentFormError.empty()) {
        sf::Text err(font, treatmentFormError, 13);
        err.setFillColor(Theme::DANGER); err.setPosition({490, 430}); window.draw(err);
    }
    btnSaveT.draw(window); btnCancelT.draw(window);
}

// =============================================================================
//  Billing Screen
// =============================================================================
TextField fbPid({490, 260}, {350, 40}, "Patient ID");
TextField fbDid({490, 330}, {350, 40}, "Doctor ID");
Button btnBackB  ("<- Back", {1130, 90}, {120, 38},
                  Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
Button btnLookupB("Calculate", {860, 295}, {120, 40});
Button btnChargeB("Charge & Pay", {490, 580}, {220, 48},
                  Theme::SUCCESS, sf::Color(74,222,128), Theme::SUCCESS_DARK);
Button btnClearB ("Reset", {720, 580}, {120, 48},
                  Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
struct BillState {
    bool valid = false;
    int patientIdx = -1; int doctorIdx = -1; int treatmentIdx = -1;
    double treatmentCost = 0.0; double totalCharge = 0.0;
    std::string error; std::string successMsg;
};
BillState bill;
void resetBillingScreen() {
    fbPid.clear(); fbDid.clear();
    fbPid.numeric = true; fbPid.maxLen = 9;
    fbDid.numeric = true; fbDid.maxLen = 9;
    bill = BillState{};
}
void recomputeBill() {
    bill = BillState{};
    if (fbPid.value.empty()) { bill.error = "Enter a Patient ID"; return; }
    int pid; try { pid = std::stoi(fbPid.value); } catch (...) { bill.error = "Patient ID must be numeric"; return; }
    int pidx = findPatientIndex(pid);
    if (pidx == -1) { bill.error = "No patient with ID " + std::to_string(pid); return; }
    if (fbDid.value.empty()) { bill.error = "Enter a Doctor ID"; return; }
    int did; try { did = std::stoi(fbDid.value); } catch (...) { bill.error = "Doctor ID must be numeric"; return; }
    int didx = -1;
    for (size_t i = 0; i < doctors.size(); ++i) if (doctors[i].docid == did) { didx = (int)i; break; }
    if (didx == -1) { bill.error = "No doctor with ID " + std::to_string(did); return; }
    bill.patientIdx = pidx; bill.doctorIdx = didx;
    bill.treatmentIdx = findPendingTreatment(pid);
    if (bill.treatmentIdx != -1) bill.treatmentCost = treatments[bill.treatmentIdx].cost;
    bill.totalCharge = CONSULTATION_FEE + bill.treatmentCost;
    bill.valid = true;
}
void applyCharge() {
    if (!bill.valid) return;
    Patient& p = patients[bill.patientIdx];
    if (p.balance < bill.totalCharge) {
        bill.error = "Insufficient balance. Need " + moneyStr(bill.totalCharge) +
                     ", have " + moneyStr(p.balance);
        return;
    }
    p.balance -= bill.totalCharge;
    if (bill.treatmentIdx != -1) treatments[bill.treatmentIdx].paid = true;
    savePatients(); saveTreatments(); saveBills();
    Appointment a;
    a.patientid = p.patientid; a.doctorid = doctors[bill.doctorIdx].docid;
    a.date = todayDate(); a.time = fixTime("12:00");
    appointments.push_back(a); saveAppointments();
    bill.successMsg = "Payment of " + moneyStr(bill.totalCharge) + " processed. New balance: " + moneyStr(p.balance);
    showToast("Bill processed: " + moneyStr(bill.totalCharge) + " charged");
    bill = BillState{}; fbPid.clear(); fbDid.clear();
}
void drawBilling(sf::RenderWindow& window) {
    drawTopBar(window);
    sf::Text title(font, "Process Billing", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    sf::Text sub(font, "Consultation fee " + moneyStr(CONSULTATION_FEE) + " + any pending treatment", 14);
    sub.setFillColor(Theme::TEXT_SECONDARY); sub.setPosition({80, 140}); window.draw(sub);
    btnBackB.draw(window);
    sf::RectangleShape panel({900.f, 470.f}); panel.setPosition({190.f, 200.f});
    panel.setFillColor(Theme::BG_PANEL); panel.setOutlineThickness(1.f);
    panel.setOutlineColor(Theme::BORDER); window.draw(panel);
    drawFormLabel(window, "Patient ID:", 260);
    drawFormLabel(window, "Doctor ID:",  330);
    fbPid.draw(window); fbDid.draw(window); btnLookupB.draw(window);
    auto showLookup = [&](const std::string& v, float y, bool isPatient) {
        if (v.empty()) return;
        try {
            int id = std::stoi(v);
            std::string name = isPatient ? getPatientName(id) : getDoctorName(id);
            bool exists = isPatient ? patientExists(id) : doctorExists(id);
            sf::Text t(font, exists ? (isPatient ? name : "Dr. " + name) : "(no match)", 12);
            t.setFillColor(exists ? Theme::ACCENT_HOVER : Theme::DANGER);
            t.setPosition({500, y + 50}); window.draw(t);
        } catch (...) {}
    };
    showLookup(fbPid.value, 260, true);
    showLookup(fbDid.value, 330, false);
    if (bill.valid) {
        const Patient& p = patients[bill.patientIdx]; const Doctor& d = doctors[bill.doctorIdx];
        sf::RectangleShape recap({700.f, 180.f}); recap.setPosition({290.f, 390.f});
        recap.setFillColor(Theme::BG_INPUT); recap.setOutlineThickness(1.f);
        recap.setOutlineColor(Theme::ACCENT); window.draw(recap);
        sf::RectangleShape stripe({4.f, 180.f}); stripe.setPosition({290.f, 390.f});
        stripe.setFillColor(Theme::ACCENT); window.draw(stripe);
        sf::Text heading(font, "Bill summary", 16);
        heading.setFillColor(Theme::TEXT_PRIMARY); heading.setPosition({310, 405}); window.draw(heading);
        auto drawRow = [&](const std::string& l, const std::string& r, float y, sf::Color c) {
            sf::Text lt(font, l, 13); lt.setFillColor(Theme::TEXT_SECONDARY);
            lt.setPosition({310, y}); window.draw(lt);
            sf::Text rt(font, r, 13); rt.setFillColor(c);
            sf::FloatRect rb = rt.getLocalBounds();
            rt.setOrigin({rb.position.x + rb.size.x, 0.f});
            rt.setPosition({970, y}); window.draw(rt);
        };
        drawRow("Patient", p.name + " (#" + std::to_string(p.patientid) + ")", 440, Theme::TEXT_PRIMARY);
        drawRow("Doctor",  "Dr. " + d.name + " - " + d.speciality, 460, Theme::TEXT_PRIMARY);
        drawRow("Consultation fee", moneyStr(CONSULTATION_FEE), 490, Theme::TEXT_PRIMARY);
        if (bill.treatmentIdx != -1) {
            const Treatment& t = treatments[bill.treatmentIdx];
            drawRow("Pending treatment: " + t.description, moneyStr(t.cost), 510, Theme::WARNING);
        } else drawRow("Pending treatment", "(none)", 510, Theme::TEXT_MUTED);
        drawRow("TOTAL", moneyStr(bill.totalCharge), 535, Theme::ACCENT_HOVER);
        drawRow("Current balance", moneyStr(p.balance),
                555, p.balance >= bill.totalCharge ? Theme::SUCCESS : Theme::DANGER);
        btnChargeB.draw(window); btnClearB.draw(window);
    }
    if (!bill.error.empty()) {
        sf::Text err(font, bill.error, 13);
        err.setFillColor(Theme::DANGER); err.setPosition({290, 660}); window.draw(err);
    }
    if (!bill.successMsg.empty()) {
        sf::Text ok(font, bill.successMsg, 13);
        ok.setFillColor(Theme::SUCCESS); ok.setPosition({290, 660}); window.draw(ok);
    }
}

// =============================================================================
//  Search & Reports Menu
// =============================================================================
Button btnBackS  ("<- Back to Main Menu", {1080, 90}, {170, 38},
                  Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);

struct SearchCard {
    std::string title; std::string description;
    Screen target; sf::Vector2f position; sf::Vector2f size;
    sf::Color accent;
};
std::vector<SearchCard> searchCards = {
    {"Search Patients",       "Find by ID or by partial name (case-insensitive)",
     Screen::SearchPatient,      {80,  200}, {360, 130}, Theme::ACCENT},
    {"Search Doctors",        "Find by ID or by speciality",
     Screen::SearchDoctor,       {460, 200}, {360, 130}, Theme::ACCENT},
    {"Treatments by Doctor",  "List every treatment linked to one doctor's patients",
     Screen::TreatmentsByDoctor, {840, 200}, {360, 130}, Theme::WARNING},
    {"Sort Doctors by Experience", "Reorder doctor list ascending by years",
     Screen::SortDoctors,        {80,  360}, {360, 130}, Theme::INFO},
    {"Overview Report",       "Revenue, outstanding, key counts",
     Screen::ReportOverview,     {460, 360}, {360, 130}, Theme::SUCCESS},
};

void drawSearchMenu(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text pageTitle(font, "Search & Reports", 28);
    pageTitle.setFillColor(Theme::TEXT_PRIMARY); pageTitle.setPosition({80, 100}); window.draw(pageTitle);
    sf::Text pageSub(font, "Find records, run reports, sort data", 14);
    pageSub.setFillColor(Theme::TEXT_SECONDARY); pageSub.setPosition({80, 140}); window.draw(pageSub);
    btnBackS.draw(window);

    for (const auto& c : searchCards) {
        sf::FloatRect cb({c.position.x, c.position.y}, {c.size.x, c.size.y});
        bool hovered = cb.contains(mouse);
        sf::RectangleShape card(c.size); card.setPosition(c.position);
        card.setFillColor(hovered ? Theme::BG_PANEL_HOVER : Theme::BG_PANEL);
        card.setOutlineThickness(hovered ? 2.f : 1.f);
        card.setOutlineColor(hovered ? c.accent : Theme::BORDER);
        window.draw(card);
        sf::RectangleShape stripe(sf::Vector2f(4.f, c.size.y));
        stripe.setPosition(c.position); stripe.setFillColor(c.accent);
        window.draw(stripe);
        sf::Text t(font, c.title, 20); t.setFillColor(Theme::TEXT_PRIMARY);
        t.setPosition({c.position.x + 24, c.position.y + 26}); window.draw(t);
        sf::Text d(font, c.description, 13); d.setFillColor(Theme::TEXT_SECONDARY);
        d.setPosition({c.position.x + 24, c.position.y + 64}); window.draw(d);
        sf::Text hint(font, hovered ? "Click to open  ->" : "->", 13);
        hint.setFillColor(hovered ? c.accent : Theme::TEXT_MUTED);
        hint.setPosition({c.position.x + 24, c.position.y + 95}); window.draw(hint);
    }

    sf::Text footer(font,
        "Tip: in search screens you can switch between ID / Name (or Speciality) tabs at the top", 12);
    footer.setFillColor(Theme::TEXT_MUTED);
    footer.setPosition({80, 680}); window.draw(footer);
}

// =============================================================================
//  Search Patient screen
// =============================================================================
TextField fsPatQuery({500, 145}, {550, 40}, "Enter Patient ID or name...");
Button btnPatSearch ("Search",  {1060, 145}, {110, 40});
Button btnBackSP    ("<- Back", {1180, 90}, {80, 38},
                     Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
// Two-mode tab
int searchPatientMode = 0;  // 0 = by ID, 1 = by Name
std::vector<int> patientSearchResults;
std::string patientSearchMsg;
float searchPatientScrollY = 0.f;

void resetSearchPatientScreen() {
    fsPatQuery.clear();
    fsPatQuery.numeric = (searchPatientMode == 0); fsPatQuery.maxLen = 50;
    patientSearchResults.clear(); patientSearchMsg.clear();
    searchPatientScrollY = 0.f;
}

void runPatientSearch() {
    patientSearchResults.clear();
    patientSearchMsg.clear();
    std::string q = fsPatQuery.value;
    if (q.empty()) { patientSearchMsg = "Enter a search term first"; return; }
    if (searchPatientMode == 0) {
        int id; try { id = std::stoi(q); } catch (...) { patientSearchMsg = "ID must be a number"; return; }
        int idx = findPatientIndex(id);
        if (idx == -1) patientSearchMsg = "No patient with ID " + q;
        else patientSearchResults.push_back(idx);
    } else {
        for (size_t i = 0; i < patients.size(); ++i)
            if (containsCI(patients[i].name, q)) patientSearchResults.push_back((int)i);
        if (patientSearchResults.empty())
            patientSearchMsg = "No patient name contains \"" + q + "\"";
    }
}

void drawSearchTabs(sf::RenderWindow& window, sf::Vector2f mouse,
                    int& mode, const char* label0, const char* label1) {
    sf::Vector2f p0(80, 145), p1(280, 145); sf::Vector2f sz(190, 40);
    auto drawTab = [&](sf::Vector2f p, int idx, const char* text) {
        sf::FloatRect r(p, sz);
        bool active = (mode == idx);
        sf::RectangleShape tab(sz); tab.setPosition(p);
        tab.setFillColor(active ? Theme::ACCENT_DARK : Theme::BG_PANEL);
        tab.setOutlineThickness(1.f);
        tab.setOutlineColor(active ? Theme::ACCENT : Theme::BORDER);
        window.draw(tab);
        sf::Text t(font, text, 14);
        t.setFillColor(active ? Theme::TEXT_PRIMARY : Theme::TEXT_SECONDARY);
        sf::FloatRect tb = t.getLocalBounds();
        t.setOrigin({tb.position.x + tb.size.x/2.f, tb.position.y + tb.size.y/2.f});
        t.setPosition({p.x + sz.x/2.f, p.y + sz.y/2.f});
        window.draw(t);
    };
    drawTab(p0, 0, label0); drawTab(p1, 1, label1);
}

void drawSearchPatient(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text title(font, "Search Patients", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    btnBackSP.draw(window);
    drawSearchTabs(window, mouse, searchPatientMode, "By ID", "By Name");
    fsPatQuery.draw(window); btnPatSearch.draw(window);
    static const float colX[] = {100, 180, 470, 560, 670, 850};
    static const char* colNames[] = {"ID", "Name", "Age", "Gender", "Contact", "Balance"};
    drawListPanel(window, colNames, colX, 6);
    if (!patientSearchMsg.empty()) {
        sf::Text msg(font, patientSearchMsg, 14);
        msg.setFillColor(Theme::TEXT_MUTED);
        sf::FloatRect mb = msg.getLocalBounds();
        msg.setOrigin({mb.position.x + mb.size.x/2.f, 0.f});
        msg.setPosition({TABLE_LEFT + TABLE_WIDTH/2.f, TABLE_TOP + 180.f});
        window.draw(msg);
    }
    const float ROWS_TOP = TABLE_TOP + 44.f;
    for (size_t i = 0; i < patientSearchResults.size(); ++i) {
        int idx = patientSearchResults[i];
        if (idx < 0 || idx >= (int)patients.size()) continue;
        float y = ROWS_TOP + i * ROW_HEIGHT - searchPatientScrollY;
        if (y + ROW_HEIGHT < ROWS_TOP || y > TABLE_BOTTOM) continue;
        sf::RectangleShape row({TABLE_WIDTH, ROW_HEIGHT}); row.setPosition({TABLE_LEFT, y});
        row.setFillColor(i % 2 == 0 ? Theme::BG_PANEL : Theme::BG_ROW_ALT);
        window.draw(row);
        const Patient& p = patients[idx];
        sf::Text c1(font, std::to_string(p.patientid), 14); c1.setFillColor(Theme::TEXT_PRIMARY); c1.setPosition({colX[0], y+12}); window.draw(c1);
        sf::Text c2(font, p.name, 14); c2.setFillColor(Theme::TEXT_PRIMARY); c2.setPosition({colX[1], y+12}); window.draw(c2);
        sf::Text c3(font, std::to_string(p.age), 14); c3.setFillColor(Theme::TEXT_SECONDARY); c3.setPosition({colX[2], y+12}); window.draw(c3);
        sf::Text c4(font, p.gender, 14); c4.setFillColor(Theme::TEXT_SECONDARY); c4.setPosition({colX[3], y+12}); window.draw(c4);
        sf::Text c5(font, p.contact, 14); c5.setFillColor(Theme::TEXT_SECONDARY); c5.setPosition({colX[4], y+12}); window.draw(c5);
        sf::Text c6(font, moneyStr(p.balance), 14); c6.setFillColor(Theme::TEXT_PRIMARY); c6.setPosition({colX[5], y+12}); window.draw(c6);
    }
    if (!patientSearchResults.empty()) {
        sf::Text n(font, std::to_string(patientSearchResults.size()) + " result" +
                         (patientSearchResults.size() == 1 ? "" : "s"), 13);
        n.setFillColor(Theme::ACCENT_HOVER);
        n.setPosition({80, 670}); window.draw(n);
    }
}

// =============================================================================
//  Search Doctor screen
// =============================================================================
TextField fsDocQuery({500, 145}, {550, 40}, "Enter Doctor ID or speciality...");
Button btnDocSearch("Search",  {1060, 145}, {110, 40});
Button btnBackSD   ("<- Back", {1180, 90}, {80, 38},
                    Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
int searchDoctorMode = 0;  // 0 = by ID, 1 = by Speciality
std::vector<int> doctorSearchResults;
std::string doctorSearchMsg;
float searchDoctorScrollY = 0.f;

void resetSearchDoctorScreen() {
    fsDocQuery.clear();
    fsDocQuery.numeric = (searchDoctorMode == 0); fsDocQuery.maxLen = 50;
    doctorSearchResults.clear(); doctorSearchMsg.clear();
    searchDoctorScrollY = 0.f;
}
void runDoctorSearch() {
    doctorSearchResults.clear();
    doctorSearchMsg.clear();
    std::string q = fsDocQuery.value;
    if (q.empty()) { doctorSearchMsg = "Enter a search term first"; return; }
    if (searchDoctorMode == 0) {
        int id; try { id = std::stoi(q); } catch (...) { doctorSearchMsg = "ID must be a number"; return; }
        for (size_t i = 0; i < doctors.size(); ++i) if (doctors[i].docid == id) { doctorSearchResults.push_back((int)i); break; }
        if (doctorSearchResults.empty()) doctorSearchMsg = "No doctor with ID " + q;
    } else {
        for (size_t i = 0; i < doctors.size(); ++i)
            if (containsCI(doctors[i].speciality, q)) doctorSearchResults.push_back((int)i);
        if (doctorSearchResults.empty())
            doctorSearchMsg = "No doctor speciality contains \"" + q + "\"";
    }
}
void drawSearchDoctor(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text title(font, "Search Doctors", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    btnBackSD.draw(window);
    drawSearchTabs(window, mouse, searchDoctorMode, "By ID", "By Speciality");
    fsDocQuery.draw(window); btnDocSearch.draw(window);
    static const float colX[] = {100, 220, 580, 950};
    static const char* colNames[] = {"ID", "Name", "Speciality", "Experience"};
    drawListPanel(window, colNames, colX, 4);
    if (!doctorSearchMsg.empty()) {
        sf::Text msg(font, doctorSearchMsg, 14);
        msg.setFillColor(Theme::TEXT_MUTED);
        sf::FloatRect mb = msg.getLocalBounds();
        msg.setOrigin({mb.position.x + mb.size.x/2.f, 0.f});
        msg.setPosition({TABLE_LEFT + TABLE_WIDTH/2.f, TABLE_TOP + 180.f}); window.draw(msg);
    }
    const float ROWS_TOP = TABLE_TOP + 44.f;
    for (size_t i = 0; i < doctorSearchResults.size(); ++i) {
        int idx = doctorSearchResults[i];
        if (idx < 0 || idx >= (int)doctors.size()) continue;
        float y = ROWS_TOP + i * ROW_HEIGHT - searchDoctorScrollY;
        if (y + ROW_HEIGHT < ROWS_TOP || y > TABLE_BOTTOM) continue;
        sf::RectangleShape row({TABLE_WIDTH, ROW_HEIGHT}); row.setPosition({TABLE_LEFT, y});
        row.setFillColor(i % 2 == 0 ? Theme::BG_PANEL : Theme::BG_ROW_ALT);
        window.draw(row);
        const Doctor& d = doctors[idx];
        sf::Text c1(font, std::to_string(d.docid), 14); c1.setFillColor(Theme::TEXT_PRIMARY); c1.setPosition({colX[0], y+12}); window.draw(c1);
        sf::Text c2(font, d.name, 14); c2.setFillColor(Theme::TEXT_PRIMARY); c2.setPosition({colX[1], y+12}); window.draw(c2);
        sf::Text c3(font, d.speciality, 14); c3.setFillColor(Theme::TEXT_SECONDARY); c3.setPosition({colX[2], y+12}); window.draw(c3);
        sf::Text c4(font, std::to_string(d.experience) + " yrs", 14); c4.setFillColor(Theme::TEXT_SECONDARY); c4.setPosition({colX[3], y+12}); window.draw(c4);
    }
    if (!doctorSearchResults.empty()) {
        sf::Text n(font, std::to_string(doctorSearchResults.size()) + " result" +
                         (doctorSearchResults.size() == 1 ? "" : "s"), 13);
        n.setFillColor(Theme::ACCENT_HOVER);
        n.setPosition({80, 670}); window.draw(n);
    }
}

// =============================================================================
//  Treatments by Doctor screen
// =============================================================================
TextField fsTbdDoc({500, 145}, {550, 40}, "Enter Doctor ID...");
Button btnTbdLoad ("Load",    {1060, 145}, {110, 40},
                   Theme::WARNING, sf::Color(253,224,71), Theme::WARNING_DARK);
Button btnBackTbd ("<- Back", {1180, 90}, {80, 38},
                   Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
struct TbdRow {
    int patientid; std::string patientname;
    std::string description; double cost; bool paid;
};
std::vector<TbdRow> tbdRows;
std::string tbdMsg;
float tbdScrollY = 0.f;
int tbdDoctorId = -1;

void resetTbdScreen() {
    fsTbdDoc.clear(); fsTbdDoc.numeric = true; fsTbdDoc.maxLen = 9;
    tbdRows.clear(); tbdMsg.clear(); tbdScrollY = 0.f; tbdDoctorId = -1;
}
void runTbdLoad() {
    tbdRows.clear(); tbdMsg.clear();
    if (fsTbdDoc.value.empty()) { tbdMsg = "Enter a Doctor ID first"; return; }
    int did; try { did = std::stoi(fsTbdDoc.value); } catch (...) { tbdMsg = "ID must be numeric"; return; }
    if (!doctorExists(did)) { tbdMsg = "No doctor with ID " + std::to_string(did); return; }
    tbdDoctorId = did;
    // For each appointment with this doctor, find all treatments for that patient
    for (const auto& a : appointments) {
        if (a.doctorid != did) continue;
        for (const auto& t : treatments) {
            if (t.patientid == a.patientid) {
                TbdRow r;
                r.patientid = a.patientid;
                r.patientname = getPatientName(a.patientid);
                r.description = t.description;
                r.cost = t.cost; r.paid = t.paid;
                tbdRows.push_back(r);
            }
        }
    }
    if (tbdRows.empty()) tbdMsg = "No treatments linked to this doctor's patients";
}
void drawTreatmentsByDoctor(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text title(font, "Treatments by Doctor", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    if (tbdDoctorId != -1) {
        sf::Text sub(font, "Showing for Dr. " + getDoctorName(tbdDoctorId) +
                            " (#" + std::to_string(tbdDoctorId) + ")", 14);
        sub.setFillColor(Theme::WARNING); sub.setPosition({80, 140}); window.draw(sub);
    }
    btnBackTbd.draw(window);
    fsTbdDoc.draw(window); btnTbdLoad.draw(window);
    static const float colX[] = {100, 220, 470, 850, 1000};
    static const char* colNames[] = {"Patient ID", "Patient", "Treatment", "Cost", "Status"};
    drawListPanel(window, colNames, colX, 5);
    if (!tbdMsg.empty()) {
        sf::Text msg(font, tbdMsg, 14);
        msg.setFillColor(Theme::TEXT_MUTED);
        sf::FloatRect mb = msg.getLocalBounds();
        msg.setOrigin({mb.position.x + mb.size.x/2.f, 0.f});
        msg.setPosition({TABLE_LEFT + TABLE_WIDTH/2.f, TABLE_TOP + 180.f}); window.draw(msg);
    }
    const float ROWS_TOP = TABLE_TOP + 44.f;
    for (size_t i = 0; i < tbdRows.size(); ++i) {
        float y = ROWS_TOP + i * ROW_HEIGHT - tbdScrollY;
        if (y + ROW_HEIGHT < ROWS_TOP || y > TABLE_BOTTOM) continue;
        sf::RectangleShape row({TABLE_WIDTH, ROW_HEIGHT}); row.setPosition({TABLE_LEFT, y});
        row.setFillColor(i % 2 == 0 ? Theme::BG_PANEL : Theme::BG_ROW_ALT);
        window.draw(row);
        const TbdRow& r = tbdRows[i];
        sf::Text c1(font, std::to_string(r.patientid), 14); c1.setFillColor(Theme::TEXT_SECONDARY); c1.setPosition({colX[0], y+12}); window.draw(c1);
        sf::Text c2(font, r.patientname, 14); c2.setFillColor(Theme::TEXT_PRIMARY); c2.setPosition({colX[1], y+12}); window.draw(c2);
        sf::Text c3(font, r.description, 14); c3.setFillColor(Theme::TEXT_PRIMARY); c3.setPosition({colX[2], y+12}); window.draw(c3);
        sf::Text c4(font, moneyStr(r.cost), 14); c4.setFillColor(Theme::TEXT_SECONDARY); c4.setPosition({colX[3], y+12}); window.draw(c4);
        drawStatusBadge(window, colX[4], y + 11, r.paid);
    }
    if (!tbdRows.empty()) {
        sf::Text n(font, std::to_string(tbdRows.size()) + " row" +
                         (tbdRows.size() == 1 ? "" : "s"), 13);
        n.setFillColor(Theme::WARNING); n.setPosition({80, 670}); window.draw(n);
    }
}

// =============================================================================
//  Sort Doctors by Experience
// =============================================================================
Button btnBackSort ("<- Back", {1180, 90}, {80, 38},
                    Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
Button btnApplySort("Apply Sort", {1010, 145}, {160, 40},
                    Theme::INFO, sf::Color(125,211,252), sf::Color(2,132,199));
float sortScrollY = 0.f;
bool  sortApplied = false;

void doSortDoctorsByExperience() {
    // Bubble sort ascending — matches your original
    for (size_t i = 0; i + 1 < doctors.size(); ++i)
        for (size_t j = 0; j + 1 < doctors.size() - i; ++j)
            if (doctors[j].experience > doctors[j+1].experience)
                std::swap(doctors[j], doctors[j+1]);
    saveDoctors();
    sortApplied = true;
    showToast("Doctors sorted by experience (ascending)", Theme::INFO);
}

void drawSortDoctors(sf::RenderWindow& window, sf::Vector2f mouse) {
    drawTopBar(window);
    sf::Text title(font, "Sort Doctors by Experience", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    sf::Text sub(font, "Apply will reorder the doctor list ascending by years of experience", 14);
    sub.setFillColor(Theme::TEXT_SECONDARY); sub.setPosition({80, 140}); window.draw(sub);
    btnBackSort.draw(window); btnApplySort.draw(window);

    static const float colX[] = {100, 220, 580, 950};
    static const char* colNames[] = {"ID", "Name", "Speciality", "Experience"};
    drawListPanel(window, colNames, colX, 4);

    if (doctors.empty()) {
        sf::Text empty(font, "No doctors loaded", 16);
        empty.setFillColor(Theme::TEXT_MUTED);
        sf::FloatRect eb = empty.getLocalBounds();
        empty.setOrigin({eb.position.x + eb.size.x/2.f, 0.f});
        empty.setPosition({TABLE_LEFT + TABLE_WIDTH/2.f, TABLE_TOP + 180.f}); window.draw(empty);
    }

    const float ROWS_TOP = TABLE_TOP + 44.f;
    for (size_t i = 0; i < doctors.size(); ++i) {
        float y = ROWS_TOP + i * ROW_HEIGHT - sortScrollY;
        if (y + ROW_HEIGHT < ROWS_TOP || y > TABLE_BOTTOM) continue;
        sf::RectangleShape row({TABLE_WIDTH, ROW_HEIGHT}); row.setPosition({TABLE_LEFT, y});
        row.setFillColor(i % 2 == 0 ? Theme::BG_PANEL : Theme::BG_ROW_ALT);
        window.draw(row);
        const Doctor& d = doctors[i];
        sf::Text c1(font, std::to_string(d.docid), 14); c1.setFillColor(Theme::TEXT_SECONDARY); c1.setPosition({colX[0], y+12}); window.draw(c1);
        sf::Text c2(font, d.name, 14); c2.setFillColor(Theme::TEXT_PRIMARY); c2.setPosition({colX[1], y+12}); window.draw(c2);
        sf::Text c3(font, d.speciality, 14); c3.setFillColor(Theme::TEXT_SECONDARY); c3.setPosition({colX[2], y+12}); window.draw(c3);
        sf::Text c4(font, std::to_string(d.experience) + " yrs", 14); c4.setFillColor(sortApplied ? Theme::INFO : Theme::TEXT_SECONDARY);
        c4.setPosition({colX[3], y+12}); window.draw(c4);
    }
    if (sortApplied) {
        sf::Text msg(font, "Sort applied and saved to doctors.txt", 13);
        msg.setFillColor(Theme::SUCCESS); msg.setPosition({80, 670}); window.draw(msg);
    }
}

// =============================================================================
//  Overview Report
// =============================================================================
Button btnBackR("<- Back", {1180, 90}, {80, 38},
                Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);

void drawStatCard(sf::RenderWindow& w, sf::Vector2f pos, sf::Vector2f size,
                  const std::string& label, const std::string& value,
                  sf::Color accent, const std::string& sub = "") {
    sf::RectangleShape card(size); card.setPosition(pos);
    card.setFillColor(Theme::BG_PANEL); card.setOutlineThickness(1.f);
    card.setOutlineColor(Theme::BORDER); w.draw(card);
    sf::RectangleShape stripe({4.f, size.y}); stripe.setPosition(pos);
    stripe.setFillColor(accent); w.draw(stripe);

    sf::Text l(font, label, 12); l.setFillColor(Theme::TEXT_SECONDARY);
    l.setPosition({pos.x + 20, pos.y + 18}); w.draw(l);
    sf::Text v(font, value, 30); v.setFillColor(Theme::TEXT_PRIMARY);
    v.setPosition({pos.x + 20, pos.y + 38}); w.draw(v);
    if (!sub.empty()) {
        sf::Text s(font, sub, 12); s.setFillColor(accent);
        s.setPosition({pos.x + 20, pos.y + size.y - 28}); w.draw(s);
    }
}

void drawReportOverview(sf::RenderWindow& window) {
    drawTopBar(window);
    sf::Text title(font, "Overview Report", 28);
    title.setFillColor(Theme::TEXT_PRIMARY); title.setPosition({80, 100}); window.draw(title);
    sf::Text sub(font, "Snapshot of the hospital database", 14);
    sub.setFillColor(Theme::TEXT_SECONDARY); sub.setPosition({80, 140}); window.draw(sub);
    btnBackR.draw(window);

    // Compute report stats
    int paidCount = 0, unpaidCount = 0;
    double revenue = 0.0, outstanding = 0.0;
    for (const auto& t : treatments) {
        if (t.paid) { paidCount++; revenue += t.cost; }
        else        { unpaidCount++; outstanding += t.cost; }
    }
    double totalBalance = 0.0;
    for (const auto& p : patients) totalBalance += p.balance;

    int avgExp = 0;
    if (!doctors.empty()) {
        int sum = 0;
        for (const auto& d : doctors) sum += d.experience;
        avgExp = sum / (int)doctors.size();
    }

    sf::Vector2f cardSize(260, 110);
    float startX = 80, startY = 200, gap = 20;

    drawStatCard(window, {startX, startY}, cardSize,
                 "PATIENTS", std::to_string(patients.size()), Theme::ACCENT,
                 "Total registered");
    drawStatCard(window, {startX + (cardSize.x + gap)*1, startY}, cardSize,
                 "DOCTORS", std::to_string(doctors.size()), Theme::INFO,
                 "Avg " + std::to_string(avgExp) + " yrs experience");
    drawStatCard(window, {startX + (cardSize.x + gap)*2, startY}, cardSize,
                 "APPOINTMENTS", std::to_string(appointments.size()), Theme::WARNING,
                 "Scheduled all time");
    drawStatCard(window, {startX + (cardSize.x + gap)*3, startY}, cardSize,
                 "TREATMENTS", std::to_string(treatments.size()), Theme::SUCCESS,
                 std::to_string(paidCount) + " paid - " + std::to_string(unpaidCount) + " unpaid");

    // Money panel
    sf::RectangleShape money({1120, 200}); money.setPosition({80, 350});
    money.setFillColor(Theme::BG_PANEL); money.setOutlineThickness(1.f);
    money.setOutlineColor(Theme::BORDER); window.draw(money);
    sf::RectangleShape mStripe({4.f, 200.f}); mStripe.setPosition({80, 350});
    mStripe.setFillColor(Theme::SUCCESS); window.draw(mStripe);

    sf::Text mHead(font, "Financial summary", 18);
    mHead.setFillColor(Theme::TEXT_PRIMARY); mHead.setPosition({110, 370}); window.draw(mHead);

    auto bigRow = [&](const std::string& l, const std::string& v, float y, sf::Color c) {
        sf::Text lt(font, l, 14); lt.setFillColor(Theme::TEXT_SECONDARY);
        lt.setPosition({110, y}); window.draw(lt);
        sf::Text vt(font, v, 18); vt.setFillColor(c);
        sf::FloatRect b = vt.getLocalBounds();
        vt.setOrigin({b.position.x + b.size.x, 0.f});
        vt.setPosition({1180, y - 3}); window.draw(vt);
    };
    bigRow("Revenue collected (sum of paid treatments)", moneyStr(revenue), 420, Theme::SUCCESS);
    bigRow("Outstanding (sum of unpaid treatments)",     moneyStr(outstanding), 450, Theme::DANGER);
    bigRow("Total patient balances on file",             moneyStr(totalBalance), 480, Theme::ACCENT_HOVER);
    bigRow("Consultation fee per visit",                 moneyStr(CONSULTATION_FEE), 510, Theme::TEXT_PRIMARY);

    // Footer hint
    sf::Text footer(font,
        "Tip: revenue figure shows the cumulative cost of treatments already marked paid", 12);
    footer.setFillColor(Theme::TEXT_MUTED);
    footer.setPosition({80, 680}); window.draw(footer);
}

// =============================================================================
//  Modal
// =============================================================================
Button modalConfirmBtn("Confirm", {0, 0}, {1, 1});
Button modalCancelBtn ("Cancel",  {0, 0}, {1, 1},
                       Theme::BG_PANEL, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
void drawModal(sf::RenderWindow& window) {
    if (!modal.active) return;
    sf::RectangleShape overlay({1280.f, 720.f});
    overlay.setFillColor(Theme::OVERLAY); window.draw(overlay);
    sf::Vector2f dlgSize{480.f, 220.f};
    sf::Vector2f dlgPos{(1280.f - dlgSize.x)/2.f, (720.f - dlgSize.y)/2.f};
    sf::RectangleShape dlg(dlgSize); dlg.setPosition(dlgPos);
    dlg.setFillColor(Theme::BG_PANEL); dlg.setOutlineThickness(2.f);
    dlg.setOutlineColor(modal.accent); window.draw(dlg);
    sf::RectangleShape stripe({4.f, dlgSize.y}); stripe.setPosition(dlgPos);
    stripe.setFillColor(modal.accent); window.draw(stripe);
    sf::Text title(font, modal.title, 20);
    title.setFillColor(Theme::TEXT_PRIMARY);
    title.setPosition({dlgPos.x + 24, dlgPos.y + 24}); window.draw(title);
    sf::Text message(font, modal.message, 14);
    message.setFillColor(Theme::TEXT_SECONDARY);
    message.setPosition({dlgPos.x + 24, dlgPos.y + 72}); window.draw(message);
    if (modal.isConfirm) {
        modalConfirmBtn = Button("Yes, confirm", {dlgPos.x + 240, dlgPos.y + 150}, {220, 44},
                                 modal.accent, Theme::DANGER_HOVER, Theme::DANGER_DARK);
        modalCancelBtn  = Button("Cancel", {dlgPos.x + 24, dlgPos.y + 150}, {200, 44},
                                 Theme::BG_INPUT, Theme::BG_PANEL_HOVER, Theme::ACCENT_DARK);
        modalConfirmBtn.draw(window); modalCancelBtn.draw(window);
    } else {
        modalConfirmBtn = Button("OK", {dlgPos.x + 180, dlgPos.y + 150}, {120, 44},
                                 modal.accent, Theme::ACCENT_HOVER, Theme::ACCENT_DARK);
        modalConfirmBtn.draw(window);
    }
}

// =============================================================================
//  Toast
// =============================================================================
void drawToast(sf::RenderWindow& window) {
    if (!toast.active) return;
    float elapsed = toast.clock.getElapsedTime().asSeconds();
    if (elapsed > toast.duration) { toast.active = false; return; }
    float alpha = 1.0f;
    if (elapsed > toast.duration - 0.6f) alpha = (toast.duration - elapsed) / 0.6f;
    sf::RectangleShape bg({420.f, 60.f});
    bg.setPosition({(1280.f - 420.f)/2.f, 640.f});
    sf::Color bgc = Theme::BG_PANEL; bgc.a = (std::uint8_t)(230 * alpha);
    bg.setFillColor(bgc); bg.setOutlineThickness(2.f);
    sf::Color oc = toast.color; oc.a = (std::uint8_t)(255 * alpha);
    bg.setOutlineColor(oc); window.draw(bg);
    sf::RectangleShape stripe({4.f, 60.f});
    stripe.setPosition({(1280.f - 420.f)/2.f, 640.f}); stripe.setFillColor(oc);
    window.draw(stripe);
    sf::Text t(font, toast.text, 14);
    sf::Color tc = Theme::TEXT_PRIMARY; tc.a = (std::uint8_t)(255 * alpha);
    t.setFillColor(tc); t.setPosition({(1280.f - 420.f)/2.f + 20.f, 660.f});
    window.draw(t);
}

// =============================================================================
//  main()
// =============================================================================
int main() {
    sf::RenderWindow window(sf::VideoMode({1280, 720}),
                            "Hospital Management System");
    window.setFramerateLimit(60);

    if (!font.openFromFile("Roboto-Regular.ttf")) {
        std::cerr << "Roboto-Regular.ttf not found, trying arial.ttf\n";
        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
            std::cerr << "Fatal: could not load any font.\n";
            return 1;
        }
    }

    cleanPatients();     loadPatients();
    cleanDoctors();      loadDoctors();
    cleanAppointments(); loadAppointments();
    loadTreatments();

    bool mouseDown = false;
    bool mouseReleasedThisFrame = false;

    while (window.isOpen()) {
        mouseReleasedThisFrame = false;
        float scrollDelta = 0.f;

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            else if (const auto* mb = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mb->button == sf::Mouse::Button::Left) mouseDown = true;
            }
            else if (const auto* mb = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mb->button == sf::Mouse::Button::Left) {
                    mouseDown = false; mouseReleasedThisFrame = true;
                }
            }
            else if (const auto* sw = event->getIf<sf::Event::MouseWheelScrolled>()) {
                scrollDelta = sw->delta;
            }
            else if (const auto* te = event->getIf<sf::Event::TextEntered>()) {
                if (modal.active) {}
                else if (currentScreen == Screen::Login) {
                    loginIdField.handleTextInput(te->unicode);
                    loginPassField.handleTextInput(te->unicode);
                }
                else if (currentScreen == Screen::PatientAdd || currentScreen == Screen::PatientEdit) {
                    fId.handleTextInput(te->unicode); fName.handleTextInput(te->unicode);
                    fAge.handleTextInput(te->unicode); fGender.handleTextInput(te->unicode);
                    fContact.handleTextInput(te->unicode); fBalance.handleTextInput(te->unicode);
                }
                else if (currentScreen == Screen::DoctorAdd || currentScreen == Screen::DoctorEdit) {
                    fdId.handleTextInput(te->unicode); fdName.handleTextInput(te->unicode);
                    fdSpec.handleTextInput(te->unicode); fdExp.handleTextInput(te->unicode);
                }
                else if (currentScreen == Screen::AppointmentAdd || currentScreen == Screen::AppointmentEdit) {
                    faPid.handleTextInput(te->unicode); faDid.handleTextInput(te->unicode);
                    faDate.handleTextInput(te->unicode); faTime.handleTextInput(te->unicode);
                }
                else if (currentScreen == Screen::TreatmentAdd || currentScreen == Screen::TreatmentEdit) {
                    ftPid.handleTextInput(te->unicode); ftDesc.handleTextInput(te->unicode);
                    ftCost.handleTextInput(te->unicode);
                }
                else if (currentScreen == Screen::Billing) {
                    fbPid.handleTextInput(te->unicode); fbDid.handleTextInput(te->unicode);
                }
                else if (currentScreen == Screen::SearchPatient) {
                    fsPatQuery.handleTextInput(te->unicode);
                }
                else if (currentScreen == Screen::SearchDoctor) {
                    fsDocQuery.handleTextInput(te->unicode);
                }
                else if (currentScreen == Screen::TreatmentsByDoctor) {
                    fsTbdDoc.handleTextInput(te->unicode);
                }
            }
            else if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (modal.active) {
                    if (kp->code == sf::Keyboard::Key::Escape) modal.active = false;
                    else if (kp->code == sf::Keyboard::Key::Enter) {
                        if (modal.isConfirm && modal.onConfirm) modal.onConfirm();
                        modal.active = false;
                    }
                }
                else if (currentScreen == Screen::Login && kp->code == sf::Keyboard::Key::Enter) {
                    attemptLogin();
                }
                else if (kp->code == sf::Keyboard::Key::Enter) {
                    if      (currentScreen == Screen::SearchPatient) runPatientSearch();
                    else if (currentScreen == Screen::SearchDoctor)  runDoctorSearch();
                    else if (currentScreen == Screen::TreatmentsByDoctor) runTbdLoad();
                }
                else if (kp->code == sf::Keyboard::Key::Escape) {
                    if      (currentScreen == Screen::PatientAdd || currentScreen == Screen::PatientEdit)     currentScreen = Screen::PatientList;
                    else if (currentScreen == Screen::DoctorAdd  || currentScreen == Screen::DoctorEdit)      currentScreen = Screen::DoctorList;
                    else if (currentScreen == Screen::AppointmentAdd || currentScreen == Screen::AppointmentEdit) currentScreen = Screen::AppointmentList;
                    else if (currentScreen == Screen::TreatmentAdd || currentScreen == Screen::TreatmentEdit) currentScreen = Screen::TreatmentList;
                    else if (currentScreen == Screen::Billing) currentScreen = Screen::TreatmentList;
                    else if (currentScreen == Screen::SearchPatient || currentScreen == Screen::SearchDoctor ||
                             currentScreen == Screen::TreatmentsByDoctor || currentScreen == Screen::SortDoctors ||
                             currentScreen == Screen::ReportOverview)
                        currentScreen = Screen::SearchMenu;
                    else if (currentScreen != Screen::Login && currentScreen != Screen::MainMenu)
                        currentScreen = Screen::MainMenu;
                }
            }
        }

        sf::Vector2i mp = sf::Mouse::getPosition(window);
        sf::Vector2f mouse(static_cast<float>(mp.x), static_cast<float>(mp.y));

        if (modal.active) {
            modalConfirmBtn.update(mouse, mouseDown);
            if (modal.isConfirm) modalCancelBtn.update(mouse, mouseDown);
            if (modalConfirmBtn.wasClicked(mouse, mouseReleasedThisFrame)) {
                if (modal.onConfirm) modal.onConfirm(); modal.active = false;
            }
            else if (modal.isConfirm && modalCancelBtn.wasClicked(mouse, mouseReleasedThisFrame)) {
                modal.active = false;
            }
        }
        else if (currentScreen == Screen::Login) {
            loginIdField.handleClick(mouse, mouseReleasedThisFrame);
            loginPassField.handleClick(mouse, mouseReleasedThisFrame);
            loginButton.update(mouse, mouseDown);
            if (loginButton.wasClicked(mouse, mouseReleasedThisFrame)) attemptLogin();
        }
        else if (currentScreen == Screen::MainMenu) {
            if (mouseReleasedThisFrame) {
                for (auto& item : menuItems) {
                    sf::FloatRect cb({item.position.x, item.position.y}, {item.size.x, item.size.y});
                    if (cb.contains(mouse)) {
                        currentScreen = item.target;
                        if (currentScreen == Screen::Login) {
                            loginIdField.clear(); loginPassField.clear(); loginMessage.clear();
                        }
                        if (currentScreen == Screen::PatientList) { selectedPatientIndex = -1; patientScrollY = 0; }
                        if (currentScreen == Screen::DoctorList)  { selectedDoctorIndex  = -1; doctorScrollY  = 0; }
                        if (currentScreen == Screen::AppointmentList) { selectedApptIndex = -1; apptScrollY = 0; }
                        if (currentScreen == Screen::TreatmentList) { selectedTreatmentIndex = -1; treatmentScrollY = 0; }
                        break;
                    }
                }
            }
        }
        else if (currentScreen == Screen::PatientList) {
            btnBackP.update(mouse, mouseDown); btnAddP.update(mouse, mouseDown);
            bool hasSel = (selectedPatientIndex >= 0 && selectedPatientIndex < (int)patients.size());
            if (hasSel) { btnEditP.update(mouse, mouseDown); btnDelP.update(mouse, mouseDown); }
            if (scrollDelta != 0.f) {
                patientScrollY -= scrollDelta * 40.f;
                if (patientScrollY < 0.f) patientScrollY = 0.f;
                float maxS = std::max(0.f, patients.size() * ROW_HEIGHT - (TABLE_BOTTOM - TABLE_TOP - 44.f));
                if (patientScrollY > maxS) patientScrollY = maxS;
            }
            if (mouseReleasedThisFrame) {
                if (btnBackP.wasClicked(mouse, true)) currentScreen = Screen::MainMenu;
                else if (btnAddP.wasClicked(mouse, true)) { isEditMode = false; resetPatientForm(); currentScreen = Screen::PatientAdd; }
                else if (hasSel && btnEditP.wasClicked(mouse, true)) { isEditMode = true; resetPatientForm(); prefillPatientForm(); currentScreen = Screen::PatientEdit; }
                else if (hasSel && btnDelP.wasClicked(mouse, true)) {
                    int idx = selectedPatientIndex; std::string name = patients[idx].name;
                    showConfirm("Delete patient?", "Are you sure you want to delete " + name + "?",
                        [idx, name]() { patients.erase(patients.begin() + idx); savePatients();
                                        selectedPatientIndex = -1; showToast("Deleted " + name); });
                }
                else handlePatientListClick(mouse);
            }
        }
        else if (currentScreen == Screen::PatientAdd || currentScreen == Screen::PatientEdit) {
            fId.handleClick(mouse, mouseReleasedThisFrame); fName.handleClick(mouse, mouseReleasedThisFrame);
            fAge.handleClick(mouse, mouseReleasedThisFrame); fGender.handleClick(mouse, mouseReleasedThisFrame);
            fContact.handleClick(mouse, mouseReleasedThisFrame); fBalance.handleClick(mouse, mouseReleasedThisFrame);
            btnSave.update(mouse, mouseDown); btnCancel.update(mouse, mouseDown);
            if (btnSave.wasClicked(mouse, mouseReleasedThisFrame)) {
                Patient p;
                if (validatePatient(p)) {
                    if (isEditMode) { for (auto& e : patients) if (e.patientid == editingId) { e = p; break; } savePatients(); showToast("Updated patient " + p.name); }
                    else { patients.push_back(p); savePatients(); showToast("Added patient " + p.name); }
                    selectedPatientIndex = -1; currentScreen = Screen::PatientList;
                }
            }
            else if (btnCancel.wasClicked(mouse, mouseReleasedThisFrame)) currentScreen = Screen::PatientList;
        }
        else if (currentScreen == Screen::DoctorList) {
            btnBackD.update(mouse, mouseDown); btnAddD.update(mouse, mouseDown);
            bool hasSel = (selectedDoctorIndex >= 0 && selectedDoctorIndex < (int)doctors.size());
            if (hasSel) { btnEditD.update(mouse, mouseDown); btnDelD.update(mouse, mouseDown); }
            if (scrollDelta != 0.f) {
                doctorScrollY -= scrollDelta * 40.f;
                if (doctorScrollY < 0.f) doctorScrollY = 0.f;
                float maxS = std::max(0.f, doctors.size() * ROW_HEIGHT - (TABLE_BOTTOM - TABLE_TOP - 44.f));
                if (doctorScrollY > maxS) doctorScrollY = maxS;
            }
            if (mouseReleasedThisFrame) {
                if (btnBackD.wasClicked(mouse, true)) currentScreen = Screen::MainMenu;
                else if (btnAddD.wasClicked(mouse, true)) { docEditMode = false; resetDoctorForm(); currentScreen = Screen::DoctorAdd; }
                else if (hasSel && btnEditD.wasClicked(mouse, true)) { docEditMode = true; resetDoctorForm(); prefillDoctorForm(); currentScreen = Screen::DoctorEdit; }
                else if (hasSel && btnDelD.wasClicked(mouse, true)) {
                    int idx = selectedDoctorIndex; std::string name = doctors[idx].name;
                    showConfirm("Delete doctor?", "Are you sure you want to delete Dr. " + name + "?",
                        [idx, name]() { doctors.erase(doctors.begin() + idx); saveDoctors();
                                        selectedDoctorIndex = -1; showToast("Deleted Dr. " + name); });
                }
                else handleDoctorListClick(mouse);
            }
        }
        else if (currentScreen == Screen::DoctorAdd || currentScreen == Screen::DoctorEdit) {
            fdId.handleClick(mouse, mouseReleasedThisFrame); fdName.handleClick(mouse, mouseReleasedThisFrame);
            fdSpec.handleClick(mouse, mouseReleasedThisFrame); fdExp.handleClick(mouse, mouseReleasedThisFrame);
            btnSaveD.update(mouse, mouseDown); btnCancelD.update(mouse, mouseDown);
            if (btnSaveD.wasClicked(mouse, mouseReleasedThisFrame)) {
                Doctor d;
                if (validateDoctor(d)) {
                    if (docEditMode) { for (auto& e : doctors) if (e.docid == docEditingId) { e = d; break; } saveDoctors(); showToast("Updated Dr. " + d.name); }
                    else { doctors.push_back(d); saveDoctors(); showToast("Added Dr. " + d.name); }
                    selectedDoctorIndex = -1; currentScreen = Screen::DoctorList;
                }
            }
            else if (btnCancelD.wasClicked(mouse, mouseReleasedThisFrame)) currentScreen = Screen::DoctorList;
        }
        else if (currentScreen == Screen::AppointmentList) {
            btnBackA.update(mouse, mouseDown); btnAddA.update(mouse, mouseDown);
            bool hasSel = (selectedApptIndex >= 0 && selectedApptIndex < (int)appointments.size());
            if (hasSel) { btnEditA.update(mouse, mouseDown); btnDelA.update(mouse, mouseDown); }
            if (scrollDelta != 0.f) {
                apptScrollY -= scrollDelta * 40.f;
                if (apptScrollY < 0.f) apptScrollY = 0.f;
                float maxS = std::max(0.f, appointments.size() * ROW_HEIGHT - (TABLE_BOTTOM - TABLE_TOP - 44.f));
                if (apptScrollY > maxS) apptScrollY = maxS;
            }
            if (mouseReleasedThisFrame) {
                if (btnBackA.wasClicked(mouse, true)) currentScreen = Screen::MainMenu;
                else if (btnAddA.wasClicked(mouse, true)) { apptEditMode = false; resetApptForm(); currentScreen = Screen::AppointmentAdd; }
                else if (hasSel && btnEditA.wasClicked(mouse, true)) { apptEditMode = true; resetApptForm(); prefillApptForm(); currentScreen = Screen::AppointmentEdit; }
                else if (hasSel && btnDelA.wasClicked(mouse, true)) {
                    int idx = selectedApptIndex; const Appointment& a = appointments[idx];
                    std::string desc = getPatientName(a.patientid) + " with Dr. " + getDoctorName(a.doctorid);
                    showConfirm("Delete appointment?", "Cancel appointment for " + desc + "?",
                        [idx]() { appointments.erase(appointments.begin() + idx); saveAppointments();
                                  selectedApptIndex = -1; showToast("Appointment cancelled"); });
                }
                else handleApptListClick(mouse);
            }
        }
        else if (currentScreen == Screen::AppointmentAdd || currentScreen == Screen::AppointmentEdit) {
            faPid.handleClick(mouse, mouseReleasedThisFrame); faDid.handleClick(mouse, mouseReleasedThisFrame);
            faDate.handleClick(mouse, mouseReleasedThisFrame); faTime.handleClick(mouse, mouseReleasedThisFrame);
            btnSaveA.update(mouse, mouseDown); btnCancelA.update(mouse, mouseDown);
            if (btnSaveA.wasClicked(mouse, mouseReleasedThisFrame)) {
                Appointment a;
                if (validateAppointment(a)) {
                    if (apptEditMode) { appointments[apptEditingIndex] = a; saveAppointments(); showToast("Updated appointment"); }
                    else { appointments.push_back(a); saveAppointments(); showToast("Appointment scheduled"); }
                    selectedApptIndex = -1; currentScreen = Screen::AppointmentList;
                }
            }
            else if (btnCancelA.wasClicked(mouse, mouseReleasedThisFrame)) currentScreen = Screen::AppointmentList;
        }
        else if (currentScreen == Screen::TreatmentList) {
            btnBackT.update(mouse, mouseDown); btnAddT.update(mouse, mouseDown); btnGoBilling.update(mouse, mouseDown);
            bool hasSel = (selectedTreatmentIndex >= 0 && selectedTreatmentIndex < (int)treatments.size());
            if (hasSel) { btnEditT.update(mouse, mouseDown); btnDelT.update(mouse, mouseDown); }
            if (scrollDelta != 0.f) {
                treatmentScrollY -= scrollDelta * 40.f;
                if (treatmentScrollY < 0.f) treatmentScrollY = 0.f;
                float maxS = std::max(0.f, treatments.size() * ROW_HEIGHT - (TABLE_BOTTOM - TABLE_TOP - 44.f));
                if (treatmentScrollY > maxS) treatmentScrollY = maxS;
            }
            if (mouseReleasedThisFrame) {
                if (btnBackT.wasClicked(mouse, true)) currentScreen = Screen::MainMenu;
                else if (btnAddT.wasClicked(mouse, true)) { treatmentEditMode = false; resetTreatmentForm(); currentScreen = Screen::TreatmentAdd; }
                else if (btnGoBilling.wasClicked(mouse, true)) { resetBillingScreen(); currentScreen = Screen::Billing; }
                else if (hasSel && btnEditT.wasClicked(mouse, true)) { treatmentEditMode = true; resetTreatmentForm(); prefillTreatmentForm(); currentScreen = Screen::TreatmentEdit; }
                else if (hasSel && btnDelT.wasClicked(mouse, true)) {
                    int idx = selectedTreatmentIndex; std::string desc = treatments[idx].description;
                    showConfirm("Delete treatment?", "Delete treatment '" + desc + "'?",
                        [idx]() { treatments.erase(treatments.begin() + idx); saveTreatments(); saveBills();
                                  selectedTreatmentIndex = -1; showToast("Treatment removed"); });
                }
                else handleTreatmentListClick(mouse);
            }
        }
        else if (currentScreen == Screen::TreatmentAdd || currentScreen == Screen::TreatmentEdit) {
            ftPid.handleClick(mouse, mouseReleasedThisFrame); ftDesc.handleClick(mouse, mouseReleasedThisFrame);
            ftCost.handleClick(mouse, mouseReleasedThisFrame);
            btnSaveT.update(mouse, mouseDown); btnCancelT.update(mouse, mouseDown);
            if (btnSaveT.wasClicked(mouse, mouseReleasedThisFrame)) {
                Treatment t; bool wasPaid = false;
                if (validateTreatment(t, wasPaid)) {
                    if (treatmentEditMode) { treatments[treatmentEditingIndex] = t; saveTreatments(); saveBills(); showToast("Treatment updated"); }
                    else { treatments.push_back(t); saveTreatments(); saveBills(); showToast("Treatment added"); }
                    selectedTreatmentIndex = -1; currentScreen = Screen::TreatmentList;
                }
            }
            else if (btnCancelT.wasClicked(mouse, mouseReleasedThisFrame)) currentScreen = Screen::TreatmentList;
        }
        else if (currentScreen == Screen::Billing) {
            fbPid.handleClick(mouse, mouseReleasedThisFrame); fbDid.handleClick(mouse, mouseReleasedThisFrame);
            btnBackB.update(mouse, mouseDown); btnLookupB.update(mouse, mouseDown);
            if (bill.valid) { btnChargeB.update(mouse, mouseDown); btnClearB.update(mouse, mouseDown); }
            if (btnBackB.wasClicked(mouse, mouseReleasedThisFrame)) currentScreen = Screen::TreatmentList;
            else if (btnLookupB.wasClicked(mouse, mouseReleasedThisFrame)) recomputeBill();
            else if (bill.valid && btnChargeB.wasClicked(mouse, mouseReleasedThisFrame)) {
                showConfirm("Process this charge?",
                    "Charge " + moneyStr(bill.totalCharge) + " to " + patients[bill.patientIdx].name + "?",
                    []() { applyCharge(); }, Theme::SUCCESS);
            }
            else if (bill.valid && btnClearB.wasClicked(mouse, mouseReleasedThisFrame)) resetBillingScreen();
        }
        else if (currentScreen == Screen::SearchMenu) {
            btnBackS.update(mouse, mouseDown);
            if (mouseReleasedThisFrame) {
                if (btnBackS.wasClicked(mouse, true)) currentScreen = Screen::MainMenu;
                else {
                    for (const auto& c : searchCards) {
                        sf::FloatRect cb({c.position.x, c.position.y}, {c.size.x, c.size.y});
                        if (cb.contains(mouse)) {
                            currentScreen = c.target;
                            if (currentScreen == Screen::SearchPatient) resetSearchPatientScreen();
                            if (currentScreen == Screen::SearchDoctor)  resetSearchDoctorScreen();
                            if (currentScreen == Screen::TreatmentsByDoctor) resetTbdScreen();
                            if (currentScreen == Screen::SortDoctors) sortApplied = false;
                            break;
                        }
                    }
                }
            }
        }
        else if (currentScreen == Screen::SearchPatient) {
            fsPatQuery.handleClick(mouse, mouseReleasedThisFrame);
            btnPatSearch.update(mouse, mouseDown); btnBackSP.update(mouse, mouseDown);
            if (scrollDelta != 0.f) {
                searchPatientScrollY -= scrollDelta * 40.f;
                if (searchPatientScrollY < 0.f) searchPatientScrollY = 0.f;
                float maxS = std::max(0.f, patientSearchResults.size() * ROW_HEIGHT - (TABLE_BOTTOM - TABLE_TOP - 44.f));
                if (searchPatientScrollY > maxS) searchPatientScrollY = maxS;
            }
            if (mouseReleasedThisFrame) {
                // Tab clicks
                sf::FloatRect tab0({80, 145}, {190, 40}), tab1({280, 145}, {190, 40});
                if (tab0.contains(mouse) && searchPatientMode != 0) {
                    searchPatientMode = 0; fsPatQuery.numeric = true; fsPatQuery.clear();
                    patientSearchResults.clear(); patientSearchMsg.clear();
                }
                else if (tab1.contains(mouse) && searchPatientMode != 1) {
                    searchPatientMode = 1; fsPatQuery.numeric = false; fsPatQuery.clear();
                    patientSearchResults.clear(); patientSearchMsg.clear();
                }
                else if (btnPatSearch.wasClicked(mouse, true)) runPatientSearch();
                else if (btnBackSP.wasClicked(mouse, true))    currentScreen = Screen::SearchMenu;
            }
        }
        else if (currentScreen == Screen::SearchDoctor) {
            fsDocQuery.handleClick(mouse, mouseReleasedThisFrame);
            btnDocSearch.update(mouse, mouseDown); btnBackSD.update(mouse, mouseDown);
            if (scrollDelta != 0.f) {
                searchDoctorScrollY -= scrollDelta * 40.f;
                if (searchDoctorScrollY < 0.f) searchDoctorScrollY = 0.f;
                float maxS = std::max(0.f, doctorSearchResults.size() * ROW_HEIGHT - (TABLE_BOTTOM - TABLE_TOP - 44.f));
                if (searchDoctorScrollY > maxS) searchDoctorScrollY = maxS;
            }
            if (mouseReleasedThisFrame) {
                sf::FloatRect tab0({80, 145}, {190, 40}), tab1({280, 145}, {190, 40});
                if (tab0.contains(mouse) && searchDoctorMode != 0) {
                    searchDoctorMode = 0; fsDocQuery.numeric = true; fsDocQuery.clear();
                    doctorSearchResults.clear(); doctorSearchMsg.clear();
                }
                else if (tab1.contains(mouse) && searchDoctorMode != 1) {
                    searchDoctorMode = 1; fsDocQuery.numeric = false; fsDocQuery.clear();
                    doctorSearchResults.clear(); doctorSearchMsg.clear();
                }
                else if (btnDocSearch.wasClicked(mouse, true)) runDoctorSearch();
                else if (btnBackSD.wasClicked(mouse, true))    currentScreen = Screen::SearchMenu;
            }
        }
        else if (currentScreen == Screen::TreatmentsByDoctor) {
            fsTbdDoc.handleClick(mouse, mouseReleasedThisFrame);
            btnTbdLoad.update(mouse, mouseDown); btnBackTbd.update(mouse, mouseDown);
            if (scrollDelta != 0.f) {
                tbdScrollY -= scrollDelta * 40.f;
                if (tbdScrollY < 0.f) tbdScrollY = 0.f;
                float maxS = std::max(0.f, tbdRows.size() * ROW_HEIGHT - (TABLE_BOTTOM - TABLE_TOP - 44.f));
                if (tbdScrollY > maxS) tbdScrollY = maxS;
            }
            if (mouseReleasedThisFrame) {
                if (btnTbdLoad.wasClicked(mouse, true))      runTbdLoad();
                else if (btnBackTbd.wasClicked(mouse, true)) currentScreen = Screen::SearchMenu;
            }
        }
        else if (currentScreen == Screen::SortDoctors) {
            btnBackSort.update(mouse, mouseDown); btnApplySort.update(mouse, mouseDown);
            if (scrollDelta != 0.f) {
                sortScrollY -= scrollDelta * 40.f;
                if (sortScrollY < 0.f) sortScrollY = 0.f;
                float maxS = std::max(0.f, doctors.size() * ROW_HEIGHT - (TABLE_BOTTOM - TABLE_TOP - 44.f));
                if (sortScrollY > maxS) sortScrollY = maxS;
            }
            if (mouseReleasedThisFrame) {
                if (btnBackSort.wasClicked(mouse, true))   currentScreen = Screen::SearchMenu;
                else if (btnApplySort.wasClicked(mouse, true)) doSortDoctorsByExperience();
            }
        }
        else if (currentScreen == Screen::ReportOverview) {
            btnBackR.update(mouse, mouseDown);
            if (mouseReleasedThisFrame) {
                if (btnBackR.wasClicked(mouse, true)) currentScreen = Screen::SearchMenu;
            }
        }

        window.clear(Theme::BG_DARK);

        switch (currentScreen) {
            case Screen::Login:              drawLoginScreen(window); break;
            case Screen::MainMenu:           drawMainMenu(window, mouse); break;
            case Screen::PatientList:        drawPatientList(window, mouse); break;
            case Screen::PatientAdd:         drawPatientForm(window); break;
            case Screen::PatientEdit:        drawPatientForm(window); break;
            case Screen::DoctorList:         drawDoctorList(window, mouse); break;
            case Screen::DoctorAdd:          drawDoctorForm(window); break;
            case Screen::DoctorEdit:         drawDoctorForm(window); break;
            case Screen::AppointmentList:    drawApptList(window, mouse); break;
            case Screen::AppointmentAdd:     drawApptForm(window); break;
            case Screen::AppointmentEdit:    drawApptForm(window); break;
            case Screen::TreatmentList:      drawTreatmentList(window, mouse); break;
            case Screen::TreatmentAdd:       drawTreatmentForm(window); break;
            case Screen::TreatmentEdit:      drawTreatmentForm(window); break;
            case Screen::Billing:            drawBilling(window); break;
            case Screen::SearchMenu:         drawSearchMenu(window, mouse); break;
            case Screen::SearchPatient:      drawSearchPatient(window, mouse); break;
            case Screen::SearchDoctor:       drawSearchDoctor(window, mouse); break;
            case Screen::TreatmentsByDoctor: drawTreatmentsByDoctor(window, mouse); break;
            case Screen::SortDoctors:        drawSortDoctors(window, mouse); break;
            case Screen::ReportOverview:     drawReportOverview(window); break;
            case Screen::Exit:               window.close(); break;
        }

        drawModal(window); drawToast(window);
        window.display();
    }
    return 0;
}