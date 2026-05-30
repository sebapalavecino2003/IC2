/**
 * Chip de estado visual con icono de círculo coloreado.
 *
 * Componente reutilizable para mostrar estados online/offline/warning/
 * critical en toda la interfaz. El círculo y el borde toman el color
 * correspondiente según la función statusColor.
 */
import { Chip } from '@mui/material'
import CircleIcon from '@mui/icons-material/Circle'
import { statusColor } from '../utils/formatters'

export default function StatusChip({ status, label, size = 'small' }) {
  const color = statusColor(status)
  return (
    <Chip
      icon={<CircleIcon sx={{ fontSize: size === 'small' ? 10 : 14, color }} />}
      label={label || status}
      size={size}
      sx={{
        fontWeight: 600,
        color,
        borderColor: color,
        bgcolor: `${color}15`,
        '& .MuiChip-icon': { color },
      }}
      variant="outlined"
    />
  )
}
