# Dockerfile

# Use a C++ environment with GCC
FROM gcc:latest

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends cmake libboost-system-dev libboost-thread-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Download and install nlohmann/json header-only library
RUN mkdir -p /usr/include/nlohmann && \
    curl -L https://github.com/nlohmann/json/releases/latest/download/json.hpp -o /usr/include/nlohmann/json.hpp

# Set the working directory
WORKDIR /app

# Copy source files
COPY . .

# Build the application
RUN g++ -std=c++17 -o signaling_server signaling_server.cpp -lboost_system -lpthread

# Expose the signaling server port
EXPOSE 8080

# Run the signaling server
CMD ["./signaling_server"]
