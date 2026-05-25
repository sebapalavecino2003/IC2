import { ToggleButton, ToggleButtonGroup } from '@mui/material'

export type TimeRange = '1h' | '6h' | '24h' | '7d'

interface TimeFilterProps {
  value: TimeRange
  onChange: (range: TimeRange) => void
}

const OPTIONS: { value: TimeRange; label: string }[] = [
  { value: '1h', label: '1H' },
  { value: '6h', label: '6H' },
  { value: '24h', label: '24H' },
  { value: '7d', label: '7D' },
]

export default function TimeFilter({ value, onChange }: TimeFilterProps) {
  return (
    <ToggleButtonGroup
      value={value}
      exclusive
      onChange={(_, val) => val && onChange(val)}
      size="small"
      sx={{ mb: 2 }}
    >
      {OPTIONS.map((opt) => (
        <ToggleButton key={opt.value} value={opt.value}>
          {opt.label}
        </ToggleButton>
      ))}
    </ToggleButtonGroup>
  )
}
