FROM python:3.11-slim
WORKDIR /app
RUN pip install flask flask-cors requests
COPY app.py .
EXPOSE 8081
CMD ["python", "app.py"]
