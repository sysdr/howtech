FROM golang:1.21-alpine AS builder
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN go build -o gc-monitor cmd/monitor/main.go

FROM alpine:latest
RUN apk add --no-cache ca-certificates curl
WORKDIR /root/
COPY --from=builder /app/gc-monitor .
EXPOSE 8080
CMD ["./gc-monitor"]
