FROM node:18-alpine AS builder

WORKDIR /app

# Copy package files
COPY package.json tailwind.config.js ./
COPY public/input.css ./public/
COPY public/app.js ./public/

# Install dependencies and build
RUN npm install && \
    npm run build:css && \
    npm run build:js

# Production stage
FROM nginx:alpine

# Copy built files
COPY public /usr/share/nginx/html
COPY --from=builder /app/public/tailwind.css /usr/share/nginx/html/tailwind.css
COPY --from=builder /app/public/app.compiled.js /usr/share/nginx/html/app.compiled.js

EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
