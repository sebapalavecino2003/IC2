import { createTheme } from '@mui/material/styles'

const industrialDarkTheme = createTheme({
  palette: {
    mode: 'dark',
    primary: { main: '#00BFA5' },
    secondary: { main: '#FF6D00' },
    background: {
      default: '#0D1117',
      paper: '#161B22',
    },
    text: {
      primary: '#E6EDF3',
      secondary: '#8B949E',
    },
    divider: '#30363D',
  },
  typography: {
    fontFamily: "'Roboto Mono', 'Roboto', monospace",
  },
  shape: {
    borderRadius: 8,
  },
  components: {
    MuiCard: {
      styleOverrides: {
        root: {
          backgroundColor: '#1C2333',
          border: '1px solid #30363D',
        },
      },
    },
  },
})

export default industrialDarkTheme
