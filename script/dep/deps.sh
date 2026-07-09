#!/bin/bash

# BlackLine/LIDE Dependency Installer
# This script installs all required dependencies for building and running BlackLine

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect OS and distribution
# Enhanced detection: prefers /etc/os-release but falls back to ID_LIKE if present
detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        # Prefer the explicit ID, but if it is not one of the known families,
        # fall back to ID_LIKE which may contain "debian", "arch", "fedora" etc.
        OS=$ID
        VERSION=$VERSION_ID
        if [[ ! "$OS" =~ ^(debian|ubuntu|kali|raspbian|fedora|arch|macos)$ ]]; then
            if [ -n "$ID_LIKE" ]; then
                if echo "$ID_LIKE" | grep -qi debian; then
                    OS="debian"
                elif echo "$ID_LIKE" | grep -qi fedora; then
                    OS="fedora"
                elif echo "$ID_LIKE" | grep -qi arch; then
                    OS="arch"
                fi
            fi
        fi
    elif [ -f /etc/debian_version ]; then
        OS="debian"
    elif [ -f /etc/arch-release ]; then
        OS="arch"
    elif [ -f /etc/fedora-release ]; then
        OS="fedora"
    elif [ "$(uname)" = "Darwin" ]; then
        OS="macos"
    else
        OS="unknown"
    fi
    print_status "Detected OS: $OS"
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        print_error "Please run as root (use sudo)"
        exit 1
    fi
}

# Update package manager
update_packages() {
    print_status "Updating package manager..."
    case $OS in
        debian|ubuntu|kali)
            apt-get update -y
            ;;
        fedora)
            dnf check-update -y || true
            ;;
        arch)
            pacman -Sy --noconfirm
            ;;
        *)
            print_warning "Cannot update packages for unknown OS"
            ;;
    esac
}

# Helper for Debian/Ubuntu package installation that tolerates missing package names
apt_install() {
    # Attempts to install the full list; on failure tries packages individually
    PKGS=("$@")
    print_status "Installing: ${PKGS[*]}"
    set +e
    apt-get install -y "${PKGS[@]}"
    rc=$?
    set -e
    if [ $rc -ne 0 ]; then
        print_warning "Batch apt install failed; trying packages individually"
        for p in "${PKGS[@]}"; do
            set +e
            apt-get install -y "$p"
            if [ $? -ne 0 ]; then
                print_warning "Package $p not available or failed to install"
            fi
            set -e
        done
    fi
}

# Install dependencies for Debian/Ubuntu/Kali
install_debian() {
    print_status "Installing dependencies for Debian/Ubuntu/Kali..."
    
    # Base development tools
    apt_install \
        build-essential \
        pkg-config \
        git \
        cmake \
        make \
        gcc \
        g++ \
        libc6-dev \
        autoconf \
        automake \
        libtool \
        meson \
        ninja-build \
        valgrind \
        gdb \
        strace

    # X11 and window manager dependencies
    apt_install \
        libx11-dev \
        libxft-dev \
        libxinerama-dev \
        libxrandr-dev \
        libxcursor-dev \
        libxi-dev \
        libxext-dev \
        libxcomposite-dev \
        libxdamage-dev \
        libxfixes-dev \
        libxrender-dev \
        libxss-dev \
        libxtst-dev \
        x11proto-core-dev \
        x11proto-xinerama-dev \
        libxcb1-dev \
        libxcb-util0-dev \
        libxcb-icccm4-dev \
        libxcb-keysyms1-dev \
        libxcb-randr0-dev \
        libxcb-shape0-dev \
        libxcb-xinerama0-dev \
        libxcb-xkb-dev

    # GTK and GUI dependencies
    apt_install \
        libgtk-3-dev \
        libgdk-pixbuf2.0-dev \
        libglib2.0-dev \
        libcairo2-dev \
        libpango1.0-dev \
        libatk1.0-dev \
        libharfbuzz-dev \
        libfontconfig1-dev \
        libfreetype6-dev \
        libepoxy-dev

    # WebKitGTK (critical for browser)
    apt_install \
        libwebkit2gtk-4.1-dev \
        libjavascriptcoregtk-4.1-dev \
        libsoup-3.0-dev \
        libgcrypt20-dev \
        libsecret-1-dev

    # Image and graphics libraries (Imlib2 for WM wallpaper)
    apt_install \
        libimlib2-dev \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        libgif-dev \
        libwebp-dev \
        librsvg2-dev

    # Terminal dependencies (VTE)
    apt_install \
        libvte-2.91-dev \
        libpcre2-dev

    # System monitoring
    apt_install \
        libgtop2-dev \
        libprocps-dev \
        libsensors-dev \
        libudev-dev

    # NetworkManager dependencies
    apt_install \
        libnm-dev \
        libnl-3-dev \
        libnl-genl-3-dev

    # PulseAudio (critical for sound settings)
    apt_install \
        libpulse-dev \
        pulseaudio \
        pavucontrol \
        alsa-utils \
        libasound2-dev \
        speaker-test

    # Xephyr (critical for testing)
    apt_install \
        xserver-xephyr \
        xorg \
        x11-utils \
        x11-xserver-utils \
        xinit \
        xterm \
        xdotool \
        wmctrl

    # Utilities
    apt_install \
        curl \
        wget \
        nano \
        vim \
        htop \
        feh || print_warning "Some utility packages failed to install"

    # Optional X utilities referenced by code (install if available)
    apt_install xbacklight || print_warning "xbacklight not available"
    apt_install brightnessctl || print_warning "brightnessctl not available"
    apt_install xprintidle || print_warning "xprintidle not available"
    apt_install xss-lock || print_warning "xss-lock not available"
    apt_install geoclue-2.0 || apt_install geoclue || print_warning "geoclue not available"
    apt_install xinput || print_warning "xinput not available"

    print_success "Debian/Ubuntu/Kali dependencies installed"
}

# Install dependencies for Fedora
install_fedora() {
    print_status "Installing dependencies for Fedora..."
    
    # Base development tools
    dnf install -y \
        gcc \
        gcc-c++ \
        make \
        cmake \
        git \
        pkgconfig \
        kernel-devel \
        autoconf \
        automake \
        libtool \
        meson \
        ninja-build \
        valgrind \
        gdb \
        strace

    # X11 and window manager dependencies
    dnf install -y \
        libX11-devel \
        libXft-devel \
        libXinerama-devel \
        libXrandr-devel \
        libXcursor-devel \
        libXi-devel \
        libXext-devel \
        libXcomposite-devel \
        libXdamage-devel \
        libXfixes-devel \
        libXrender-devel \
        libXScrnSaver-devel \
        libXtst-devel \
        libxcb-devel \
        xcb-util-devel \
        xcb-util-keysyms-devel \
        xcb-util-wm-devel

    # GTK and GUI dependencies
    dnf install -y \
        gtk3-devel \
        gdk-pixbuf2-devel \
        glib2-devel \
        cairo-devel \
        pango-devel \
        atk-devel \
        harfbuzz-devel \
        fontconfig-devel \
        freetype-devel \
        libepoxy-devel

    # WebKitGTK (critical for browser)
    dnf install -y \
        webkit2gtk4.1-devel \
        javascriptcoregtk4.1-devel \
        libsoup3-devel \
        libgcrypt-devel \
        libsecret-devel

    # Image and graphics libraries (Imlib2 for WM wallpaper)
    dnf install -y \
        imlib2-devel \
        libjpeg-turbo-devel \
        libpng-devel \
        libtiff-devel \
        giflib-devel \
        libwebp-devel \
        librsvg2-devel

    # Terminal dependencies
    dnf install -y \
        vte291-devel \
        pcre2-devel

    # System monitoring
    dnf install -y \
        libgtop2-devel \
        procps-ng-devel \
        lm_sensors-devel \
        libudev-devel

    # NetworkManager dependencies
    dnf install -y \
        NetworkManager-libnm-devel \
        libnl3-devel

    # PulseAudio (critical for sound settings)
    dnf install -y \
        pulseaudio-libs-devel \
        pulseaudio \
        pavucontrol \
        alsa-utils \
        alsa-lib-devel

    # Xephyr (critical for testing)
    dnf install -y \
        xorg-x11-server-Xephyr \
        xorg-x11-utils \
        xorg-x11-xinit \
        xterm \
        xdotool \
        wmctrl

    # Utilities
    dnf install -y \
        curl \
        wget \
        nano \
        vim \
        htop \
        feh || print_warning "Some utility packages failed to install"

    # Optional X utilities referenced by code (install if available)
    dnf install -y xbacklight || print_warning "xbacklight not available"
    dnf install -y brightnessctl || print_warning "brightnessctl not available"
    dnf install -y xprintidle || print_warning "xprintidle not available"
    dnf install -y xss-lock || print_warning "xss-lock not available"
    dnf install -y geoclue2 || dnf install -y geoclue || print_warning "geoclue not available"
    dnf install -y xinput || print_warning "xinput not available"

    print_success "Fedora dependencies installed"
}

# Install dependencies for Arch Linux
install_arch() {
    print_status "Installing dependencies for Arch Linux..."
    
    # Base development tools
    pacman -S --noconfirm \
        base-devel \
        gcc \
        make \
        cmake \
        git \
        pkg-config \
        autoconf \
        automake \
        libtool \
        meson \
        ninja \
        valgrind \
        gdb \
        strace

    # X11 and window manager dependencies
    pacman -S --noconfirm \
        libx11 \
        libxft \
        libxinerama \
        libxrandr \
        libxcursor \
        libxi \
        libxext \
        libxcomposite \
        libxdamage \
        libxfixes \
        libxrender \
        libxss \
        libxtst \
        xorgproto \
        libxcb \
        xcb-util \
        xcb-util-keysyms \
        xcb-util-wm

    # GTK and GUI dependencies
    pacman -S --noconfirm \
        gtk3 \
        gdk-pixbuf2 \
        glib2 \
        cairo \
        pango \
        atk \
        harfbuzz \
        fontconfig \
        freetype2 \
        libepoxy

    # WebKitGTK (critical for browser)
    pacman -S --noconfirm \
        webkit2gtk \
        webkit2gtk-4.1 \
        javascriptcoregtk \
        libsoup3 \
        libgcrypt \
        libsecret

    # Image and graphics libraries (Imlib2 for WM wallpaper)
    pacman -S --noconfirm \
        imlib2 \
        libjpeg-turbo \
        libpng \
        libtiff \
        giflib \
        libwebp \
        librsvg

    # Terminal dependencies
    pacman -S --noconfirm \
        vte3 \
        pcre2

    # System monitoring
    pacman -S --noconfirm \
        libgtop \
        procps-ng \
        lm_sensors \
        libudev0-shim

    # NetworkManager dependencies
    pacman -S --noconfirm \
        networkmanager \
        libnm \
        libnl

    # PulseAudio (critical for sound settings)
    pacman -S --noconfirm \
        pulseaudio \
        pulseaudio-alsa \
        pavucontrol \
        alsa-utils \
        alsa-lib \
        libpulse

    # Xephyr (critical for testing)
    pacman -S --noconfirm \
        xorg-server-xephyr \
        xorg-xrandr \
        xorg-xinit \
        xterm \
        xdotool \
        wmctrl

    # Utilities
    pacman -S --noconfirm \
        curl \
        wget \
        nano \
        vim \
        htop \
        feh || print_warning "Some utility packages failed to install"

    # Optional X utilities referenced by code (install if available)
    pacman -S --noconfirm xbacklight || print_warning "xbacklight not available"
    pacman -S --noconfirm brightnessctl || print_warning "brightnessctl not available"
    pacman -S --noconfirm xprintidle || print_warning "xprintidle not available"
    pacman -S --noconfirm xss-lock || print_warning "xss-lock not available"
    pacman -S --noconfirm geoclue || print_warning "geoclue not available"
    pacman -S --noconfirm xorg-xinput || print_warning "xinput not available"

    print_success "Arch Linux dependencies installed"
}

# Install dependencies for macOS (partial support)
install_macos() {
    print_warning "macOS support is experimental. Using Homebrew..."
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        print_error "Homebrew not found. Please install Homebrew first: https://brew.sh/"
        exit 1
    fi

    # Install dependencies
    brew install \
        pkg-config \
        gcc \
        make \
        cmake \
        git \
        autoconf \
        automake \
        libtool \
        meson \
        ninja \
        valgrind \
        gdb \
        libx11 \
        gtk+3 \
        glib \
        cairo \
        pango \
        atk \
        harfbuzz \
        webkit2gtk \
        imlib2 \
        jpeg \
        libpng \
        libtiff \
        librsvg \
        vte \
        pulseaudio \
        feh

    print_success "macOS dependencies installed (partial)"
    print_warning "Xephyr is not available on macOS. Use XQuartz instead."
}

# Install dependencies for unknown OS
install_unknown() {
    print_error "Unknown operating system. Please install dependencies manually."
    print_status "Required packages:"
    echo ""
    echo "  CRITICAL (must have):"
    echo "    - WebKitGTK (libwebkit2gtk-4.1-dev or webkit2gtk4.1-devel)"
    echo "    - PulseAudio (libpulse-dev, pulseaudio, pavucontrol)"
    echo "    - Xephyr (xserver-xephyr or xorg-x11-server-Xephyr)"
    echo "    - feh (wallpaper setter)"
    echo "    - Imlib2 (libimlib2-dev or imlib2-devel)"
    echo ""
    echo "  Core Development:"
    echo "    - build-essential, gcc, make, cmake, pkg-config, autoconf, automake, libtool, meson, ninja-build"
    echo ""
    echo "  X11 & Window Manager:"
    echo "    - libx11-dev, libxft-dev, libxinerama-dev, libxrandr-dev, libxcursor-dev, libxi-dev, libxext-dev"
    echo "    - libxcomposite-dev, libxdamage-dev, libxfixes-dev, libxrender-dev, libxss-dev, libxtst-dev"
    echo "    - libxcb1-dev, libxcb-util0-dev, libxcb-icccm4-dev, libxcb-keysyms1-dev, libxcb-randr0-dev"
    echo ""
    echo "  GTK & GUI:"
    echo "    - libgtk-3-dev, libgdk-pixbuf2.0-dev, libglib2.0-dev, libcairo2-dev"
    echo "    - libpango1.0-dev, libatk1.0-dev, libharfbuzz-dev, libfontconfig1-dev, libfreetype6-dev"
    echo ""
    echo "  Image & Graphics:"
    echo "    - libjpeg-dev, libpng-dev, libtiff-dev, librsvg2-dev"
    echo ""
    echo "  Terminal:"
    echo "    - libvte-2.91-dev, libpcre2-dev"
    echo ""
    echo "  System Monitoring:"
    echo "    - libgtop2-dev, libprocps-dev, libsensors-dev"
    echo ""
    echo "  Network:"
    echo "    - libnm-dev, libnl-3-dev"
    echo ""
    echo "  Utilities:"
    echo "    - xorg, x11-utils, x11-xserver-utils, xinit, xterm, xdotool, wmctrl"
    echo "    - curl, wget, nano, vim, htop"
    echo ""
    exit 1
}

# Verify critical installations
verify_installations() {
    print_status "Verifying critical installations..."
    
    # Check for critical headers/libraries
    check_pkg() {
        if pkg-config --exists "$1" 2>/dev/null; then
            print_success "Found $1 ($(pkg-config --modversion "$1"))"
            return 0
        else
            print_warning "Could not verify $1"
            return 1
        fi
    }
    
    # CRITICAL: WebKitGTK
    if check_pkg "webkit2gtk-4.1" || check_pkg "webkit2gtk-4.0"; then
        print_success "WebKitGTK found"
    else
        print_error "WebKitGTK NOT FOUND! Browser will not work."
    fi
    
    # CRITICAL: PulseAudio
    if check_pkg "libpulse"; then
        print_success "PulseAudio found"
    else
        print_warning "PulseAudio not found - sound settings may not work"
    fi
    
    # CRITICAL: Xephyr
    if command -v Xephyr &> /dev/null; then
        print_success "Xephyr found"
    else
        print_error "Xephyr NOT FOUND! Testing will not work."
    fi
    
    # CRITICAL: feh
    if command -v feh &> /dev/null; then
        print_success "feh found"
    else
        print_warning "feh not found - wallpaper may not work"
    fi
    
    # CRITICAL: Imlib2
    check_pkg "imlib2"
    
    # Other packages
    check_pkg "x11"
    check_pkg "gtk+-3.0"
    check_pkg "glib-2.0"
    check_pkg "cairo"
    check_pkg "vte-2.91"
    check_pkg "libnm"
    check_pkg "libgtop-2.0"
    check_pkg "alsa"
    
    # Check for executables
    check_cmd() {
        if command -v "$1" &> /dev/null; then
            print_success "Found $1"
        else
            print_warning "Could not find $1"
        fi
    }
    
    check_cmd "gcc"
    check_cmd "make"
    check_cmd "pkg-config"
    check_cmd "cmake"
    check_cmd "pulseaudio"
    check_cmd "amixer"
    check_cmd "speaker-test"
    check_cmd "xrandr"
    
    print_success "Verification complete"
}

# Main installation function
main() {
    echo "========================================="
    echo "  BlackLine/LIDE Dependency Installer"
    echo "========================================="
    
    # Check root
    check_root
    
    # Detect OS
    detect_os
    
    # Ask for confirmation
    echo
    print_warning "This will install development packages for your system."
    echo "The installation may take several minutes and use significant disk space."
    echo
    read -p "Continue? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_status "Installation cancelled."
        exit 0
    fi
    
    # Update packages
    update_packages
    
    # Install based on OS
    case $OS in
        debian|ubuntu|kali|raspbian)
            install_debian
            ;;
        fedora)
            install_fedora
            ;;
        arch)
            install_arch
            ;;
        macos)
            install_macos
            ;;
        *)
            install_unknown
            ;;
    esac
    
    # Verify installations
    verify_installations
    
    echo
    print_success "Dependency installation complete!"
    print_status "You can now build BlackLine/LIDE using 'make'"
    echo
    print_status "To run BlackLine: ./run.sh"
}

# Run main function
main "$@"