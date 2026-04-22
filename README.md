# Blink Bank ESP32 - IoT Banking System

> A full-stack IoT banking prototype with ESP32 hardware, Firebase cloud, and a modern web interface.

![Blink Bank](https://img.shields.io/badge/Platform-ESP32-blue) ![Firebase](https://img.shields.io/badge/Cloud-Firebase-orange) ![License](https://img.shields.io/badge/License-MIT-green)

## Features

### Hardware (ESP32)
- **16x2 I2C LCD** — Shows user info and balance
- **Servo Motor** — Simulates cash dispensing
- **Green LED** — Success indicator (stays ON when logged in)
- **Red LED** — Error indicator (flashes on failures)
- **WiFi** — Connects to Firebase for real-time cloud sync

### Website (Firebase Hosted)
- **Dark theme** with interactive dot grid background (CodeWords-style)
- **Custom cursor** with mix-blend-mode effect
- **B&W ripple toggle** — Click button or press `G`
- **8 pages** with hash-based routing and browser back/forward support
- **Auto-generated avatars** from user initials (like Google)
- **Profile management** — Name, email, phone, Blink ID, avatar color
- **MFA for withdrawals** — Toggle in settings, requires PIN confirmation
- **Email notifications** via EmailJS (Gmail + Outlook fallback)
- **WhatsApp alerts** for withdrawal transactions
- **Real-time balance sync** across website and ESP32

### Security
- Multi-factor authentication (MFA) toggle for withdrawals
- PIN change (requires old PIN)
- Email transaction alerts
- WhatsApp withdrawal alerts
- Route guards (protected pages require login)

## Architecture

```
[Website] <---> [Firebase RTDB] <---> [ESP32]
   |                  |                   |
   |   Write session  |  Read session     |
   |   /atm/session/  |  /atm/session/    |
   |                  |                   |
   |   Read/Write     |                   |
   |   /atm/users/    |                   |
```

## Project Structure

```
blink-bank-esp32/
├── README.md                    # This file
├── LICENSE                      # MIT License
├── .gitignore                   # Git ignore rules
│
├── website/                     # Firebase hosted web app
│   ├── public/
│   │   └── index.html           # Complete Blink Bank v2 web app
│   ├── firebase.json            # Firebase hosting config
│   └── .firebaserc              # Firebase project config
│
└── firmware/                    # ESP32 Arduino code
    └── blink_bank_atm/
        └── blink_bank_atm.ino   # Main Arduino sketch
```

## Hardware Requirements

| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32 Dev Board | 1 | Any ESP32 variant |
| 16x2 I2C LCD | 1 | Address 0x27 or 0x3F |
| Servo Motor (SG90) | 1 | For cash dispense simulation |
| Red LED | 1 | Error indicator |
| Green LED | 1 | Success indicator |
| 220Ω Resistors | 2 | One per LED |
| Breadboard | 1 | For prototyping |
| Jumper Wires | ~15 | Male-to-male and male-to-female |

## Wiring Diagram

```
ESP32 Pin    →    Component
─────────────────────────────
VIN (5V)     →    Breadboard (+) rail
GND          →    Breadboard (-) rail
GPIO 21      →    LCD SDA
GPIO 22      →    LCD SCL
GPIO 13      →    Servo Signal (Orange)
GPIO 2       →    220Ω → Red LED (+)
GPIO 4       →    220Ω → Green LED (+)

Breadboard (+) rail → LCD VCC, Servo Red wire
Breadboard (-) rail → LCD GND, Servo Brown wire, LED (-) legs
```

## Firebase Database Structure

```json
{
  "atm": {
    "users": {
      "username": {
        "pin": "1234",
        "balance": 1000,
        "name": "Amish",
        "email": "amish@gmail.com",
        "phone": "+919847222218",
        "blinkId": "BNK-482910",
        "mfaEnabled": false,
        "avatarColor": "#3b82f6",
        "notifications": {
          "email": true,
          "whatsapp": true
        }
      }
    },
    "session": {
      "active": false,
      "username": "",
      "balance": 0,
      "action": "idle",
      "amount": 0
    }
  }
}
```

## Setup Instructions

### 1. Firebase Setup
1. Create a Firebase project at [console.firebase.google.com](https://console.firebase.google.com)
2. Enable **Realtime Database** (Asia Southeast region)
3. Get your **Database Secret** from Project Settings → Service Accounts

### 2. Website Deployment
```bash
cd website
npm install -g firebase-tools    # Install Firebase CLI (if not installed)
firebase login                   # Login to Firebase
firebase init hosting            # Select your project
# Copy public/index.html to the public folder
firebase deploy                  # Deploy!
```

### 3. ESP32 Firmware
1. Install **Arduino IDE** with ESP32 board support (v3.x)
2. Install libraries via Library Manager:
   - `ESP32Servo` by Mad Hephaestus
   - `Firebase ESP Client` by mobizt
   - `LiquidCrystal I2C`
   - `ArduinoJson` v7
3. Open `firmware/blink_bank_atm/blink_bank_atm.ino`
4. Update WiFi credentials and Firebase secret
5. Upload to ESP32

### 4. EmailJS Setup (for email notifications)
1. Sign up at [emailjs.com](https://www.emailjs.com) (free)
2. Add Gmail/Outlook service
3. Create email template with variables: `to_name`, `to_email`, `transaction_type`, `amount`, `balance`, `blink_id`, `date`
4. Update Service ID, Template ID, and Public Key in `index.html`

## Website Routes

| Route | Page | Auth Required |
|-------|------|---------------|
| `#/` | Home | No |
| `#/login` | Sign In | No |
| `#/register` | Register | No |
| `#/dashboard` | Dashboard | Yes |
| `#/deposit` | Deposit | Yes |
| `#/withdraw` | Withdraw | Yes |
| `#/profile` | Edit Profile | Yes |
| `#/settings` | Settings | Yes |

## ESP32 Serial Monitor Output

```
=== Blink Bank ATM ===
LCD: OK
WiFi: Connecting..........
WiFi: OK | IP: 192.168.1.100
Firebase: OK
Server: OK

=== ATM Ready ===

[Action] login
Logged in: amish | Rs.1000
[Action] deposit
Deposited Rs.500 | New Bal: Rs.1500
[Action] withdraw
Withdrew Rs.200 | New Bal: Rs.1300
Dispensing money...
[Action] logout
User logged out.
```

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Hardware | ESP32, I2C LCD, SG90 Servo, LEDs |
| Firmware | Arduino C++ (ESP32 Core v3.x) |
| Cloud | Firebase Realtime Database + Hosting |
| Frontend | Vanilla HTML/CSS/JS |
| Notifications | EmailJS (email), WhatsApp Web API |
| Effects | HTML Canvas (dot grid), CSS animations |

## License

MIT License — See [LICENSE](LICENSE) for details.

## Author

Built with ESP32, Firebase, and creativity.
