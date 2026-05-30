/**
 * Punto de entrada de la aplicación React NodeAlert.
 *
 * Monta la aplicación en el DOM envolviéndola con los providers
 * necesarios: BrowserRouter para enrutamiento, ThemeProvider de MUI
 * con el tema oscuro personalizado, y CssBaseline para normalizar
 * estilos base del navegador.
 */
import React from 'react'
import ReactDOM from 'react-dom/client'
import { BrowserRouter } from 'react-router-dom'
import { ThemeProvider } from '@emotion/react'
import { CssBaseline } from '@mui/material'
import darkTheme from './theme/darkTheme'
import App from './App'

ReactDOM.createRoot(document.getElementById('root')).render(
  <React.StrictMode>
    <BrowserRouter>
      <ThemeProvider theme={darkTheme}>
        <CssBaseline />
        <App />
      </ThemeProvider>
    </BrowserRouter>
  </React.StrictMode>,
)
