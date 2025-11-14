FROM python:3.11-slim
WORKDIR /app
RUN pip install requests
COPY loadgen.py .
CMD ["python", "loadgen.py"]
