FROM python:3.11-slim
WORKDIR /app
RUN pip install dnslib flask flask-cors
COPY dns_server.py .
EXPOSE 53/udp 5380
CMD ["python", "dns_server.py"]
