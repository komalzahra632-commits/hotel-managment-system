#pragma once

#include <filesystem>
#include <string>
#include <vector>

enum class UserRole {
    Admin,
    Receptionist
};

struct Room {
    int id{};
    std::string type;
    double pricePerNight{};
};

struct Guest {
    int id{};
    std::string name;
    std::string phone;
    std::string email;
};

struct Reservation {
    int id{};
    int guestId{};
    int roomId{};
    std::string checkIn;
    std::string checkOut;
    std::string status;
    double pricePerNight{};
    double serviceCharges{};
    double discount{};
};

struct Payment {
    int id{};
    int reservationId{};
    double amount{};
    std::string date;
};

class HotelSystem {
public:
    explicit HotelSystem(std::filesystem::path dataDirectory);

    bool load();
    bool save() const;
    void run(UserRole role);

private:
    std::filesystem::path dataDirectory_;
    std::vector<Room> rooms_;
    std::vector<Guest> guests_;
    std::vector<Reservation> reservations_;
    std::vector<Payment> payments_;

    void showDashboard() const;
    void manageRooms();
    void registerGuest();
    void bookRoom();
    void viewReservations() const;
    void searchReservations() const;
    void cancelReservation();
    void checkInGuest();
    void checkOutGuest();
    void manageBill();
    void showRevenueReport() const;
    void listRooms() const;
    void listGuests() const;
    void saveWithMessage() const;

    Room* findRoom(int id);
    const Room* findRoom(int id) const;
    Guest* findGuest(int id);
    const Guest* findGuest(int id) const;
    Reservation* findReservation(int id);
    const Reservation* findReservation(int id) const;
    double amountPaid(int reservationId) const;
    double billTotal(const Reservation& reservation) const;
    bool roomAvailable(int roomId, const std::string& checkIn,
                       const std::string& checkOut, int ignoredReservationId = -1) const;
};
