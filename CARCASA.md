# SmartLock 4.0 — Carcasa 3D

> Documentación de diseño mecánico e impresión 3D. Para visión general ver `SMARTLOCK_4.0.md`.

---

## Archivos originales (V4.0 sin terminar)

Los STL y 3MF de la versión anterior están en:

```
C:\Users\ALIENWARE\Desktop\PROYECTOS\02_COMPLETADOS\IoT_SmartHome\PUERTARDUINO4.0\
├── casecerrv2.stl
├── CASECERRADURA.stl
├── casecerrv2.3mf
├── CASECERRADURA.3mf
└── v2\
    └── casecerrv3_PLA_0.2_3h23m.gcode
```

Estos son el punto de partida para la V4.0. Se adaptarán en Fusion 360.

---

## Flujo de diseño

1. **Fusion 360** — Adaptar modelo existente para componentes nuevos (Nextion 5", S3-EYE, PN532)
2. **Exportar STL** — Configuración: high quality, 0.1mm desviación
3. **OrcaSlicer / Cura** — Rebanar para Anycubic Kobra 2 Neo
4. **Imprimir** — Validar ajustes mecánicos

---

## Especificaciones de impresión

| Parámetro | Valor |
|---|---|
| Impresora | Anycubic Kobra 2 Neo |
| Material recomendado | PLA+ / PETG |
| Altura capa | 0.2mm (prototipo) / 0.12mm (final) |
| Relleno | 20% giroide |
| Soportes | Solo si es necesario (ángulo > 60°) |
| Adhesión | Brim si es necesario |
| Tolerancia | 0.3mm para inserts / 0.5mm para componentes |
| Post-procesado | Lijar bordes, taladrar agujeros si es necesario |

---

## Componentes a integrar en la carcasa

- [ ] Nextion 5" (recorte rectangular para pantalla)
- [ ] R503 (agujero para el sensor de huella)
- [ ] PN532 (montaje interno, no expuesto)
- [ ] ESP32 + PCB (soportes para PCB con tornillos M3)
- [ ] LED RGB (difusor pequeño)
- [ ] Buzzer (agujero para sonido)
- [ ] Botón pulsador interior
- [ ] Cámara ESP32-S3-EYE (recorte para lente)

---

## Fase 5 — Release carcasa

| Duración | 1 semana |
|---|---|
| Herramientas | Fusion 360 + OrcaSlicer |
| Costo | ~$5 USD (~$18,735 COP) en filamento |

---

## Notas

- La Kobra 2 Neo imprime PLA+ muy bien a 210°C / 60°C cama
- PETG requiere 235°C / 75°C cama, más resistente para exteriores
- Considerar inserts roscados M3 para montaje de PCB en vez de tornillo directo al plástico
- Si la carcasa V4.0 original es demasiado pequeña para la Nextion 5", toca diseñar desde cero
