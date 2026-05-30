# Dockerfile multi-etapa para el frontend NodeAlert.
# Etapa 1 (build): Compila la aplicación React con Vite.
# Etapa 2 (runtime): Sirve los archivos estáticos con nginx.
FROM node:20-alpine AS build
WORKDIR /app
COPY package.json package-lock.json ./
RUN npm ci
COPY . .
RUN npm run build

FROM nginx:alpine
RUN apk add --no-cache curl
COPY nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /app/dist /usr/share/nginx/html
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
