/**
 * Tema oscuro personalizado para NodeAlert IoT.
 *
 * Define la paleta de colores, tipografía y estilos de componentes
 * MUI. El diseño utiliza azul cian (#00e5ff) como color primario
 * para resaltar elementos interactivos, con fondos oscuros (#0a0f1a)
 * que reducen la fatiga visual en monitoreo prolongado.
 *
 * Los componentes MUI están sobrescritos para mantener consistencia:
 * bordes sutiles (#1e293b), bordes redondeados (12px) y tipografía
 * monospace para datos técnicos.
 */
import { createTheme } from '@mui/material/styles'

const darkTheme = createTheme({
  palette: {
    mode: 'dark',
    primary: {
      main: '#00e5ff',
      light: '#5effff',
      dark: '#00b2cc',
    },
    secondary: {
      main: '#ff6f00',
      light: '#ffa040',
      dark: '#c43e00',
    },
    success: {
      main: '#00e676',
      dark: '#00b248',
    },
    warning: {
      main: '#ffd600',
      dark: '#c7a500',
    },
    error: {
      main: '#ff1744',
      dark: '#c4001d',
    },
    info: {
      main: '#2979ff',
    },
    background: {
      default: '#0a0f1a',
      paper: '#111827',
    },
    text: {
      primary: '#e0e6ed',
      secondary: '#94a3b8',
    },
    divider: '#1e293b',
  },
  typography: {
    fontFamily: '"Roboto", "Helvetica", "Arial", sans-serif',
    monoFamily: '"Roboto Mono", monospace',
    h4: {
      fontWeight: 700,
      letterSpacing: 0.5,
    },
    h5: {
      fontWeight: 600,
    },
    h6: {
      fontWeight: 600,
    },
    body2: {
      fontFamily: '"Roboto Mono", monospace',
    },
  },
  shape: {
    borderRadius: 12,
  },
  components: {
    MuiCard: {
      styleOverrides: {
        root: {
          backgroundImage: 'none',
          border: '1px solid #1e293b',
          backdropFilter: 'blur(8px)',
        },
      },
    },
    MuiButton: {
      styleOverrides: {
        root: {
          textTransform: 'none',
          fontWeight: 600,
          borderRadius: 8,
        },
      },
    },
    MuiChip: {
      styleOverrides: {
        root: {
          fontWeight: 600,
        },
      },
    },
    MuiDrawer: {
      styleOverrides: {
        paper: {
          borderRight: '1px solid #1e293b',
        },
      },
    },
    MuiTableCell: {
      styleOverrides: {
        root: {
          borderBottom: '1px solid #1e293b',
        },
      },
    },
  },
})

export default darkTheme
