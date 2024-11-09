FROM gcc:latest

RUN apt-get update && \
    apt-get install -y --no-install-recommends cmake libboost-system-dev libboost-thread-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/include/nlohmann && \
    curl -L https://github.com/nlohmann/json/releases/latest/download/json.hpp -o /usr/include/nlohmann/json.hpp

WORKDIR /app

COPY . .

RUN g++ -std=c++17 -o main main.cpp -lboost_system -lpthread

EXPOSE 8321

CMD ["./main"]
