# Presupuesto de VRAM por perfil

Objetivo: que el juego quepa en una tarjeta de 4-8 GB sin paginar a RAM, y que
en una de 12 GB o más se vea a tope. **Valores de diseño, sin medir todavía**:
hay que validarlos con `memreport -full` y `stat streaming` en cada tramo.

## Elección del perfil

`PerfilArranque` en `Config/DefaultGame.ini`. Con `-1` (por defecto) lo elige
`AlsasuaEscala::PerfilEfectivo()` según la VRAM dedicada que reporta el RHI:

| VRAM dedicada | Perfil | Ejemplos |
|---|---|---|
| < 5 GB | 0 Low | GTX 1650, RX 6500 XT |
| 5 – 7,5 GB | 1 Med | RTX 2060 6 GB, RX 5600 XT |
| 7,5 – 11 GB | 2 High | RTX 3060 Ti/4060, RX 6650 XT (referencia del proyecto) |
| ≥ 11 GB | 3 Ultra | RTX 3060 12 GB, 4070, RX 6700 XT en adelante |

Para perfilar contra `RESUMEN_TECNICO.md`, fija `PerfilArranque=3`: así dos
arranques en máquinas distintas miden lo mismo.

## Qué cambia cada perfil (UAlsasuaGraphicsSettingsSubsystem)

| Partida | Low | Med | High | Ultra | Por qué pesa |
|---|---|---|---|---|---|
| Pool de texturas (`r.Streaming.PoolSize`, MB) | 800 | 1200 | 2000 | 3000 | La reserva más grande que se puede fijar; con `LimitPoolSizeToVRAM=1` degrada mips en vez de paginar |
| Tamaño máximo de textura | 512 | 1024 | 2048 | 4096 | Un mip de 4K es 4× uno de 2K |
| Páginas de sombra virtual | 512 | 1024 | 2048 | 4096 | Pool físico fijo de VSM |
| Niebla volumétrica | off | 16 px × 32 | 8 px × 64 | 8 px × 64 | Rejilla 3D fija por resolución |
| Nubes volumétricas | off | off | on | on + sombras | |
| Nanite `MaxPixelsPerEdge` | 2,0 | 1,5 | 1,0 | 1,0 | Menos píxeles por arista = más clusters; el motor no admite más de 16 M visibles |
| Multitud / tráfico | 25 % | 50 % | 75 % | 100 % | Mallas esqueléticas, animación y CPU |

Ultra tenía `MaxPixelsPerEdge=0,5`: unas 4× más clusters visibles, por encima
del tope de 16 M del motor. Al desbordar, Nanite deja de dibujar trozos y el
mundo sale con agujeros. Ahora Ultra usa 1,0, el valor por defecto de Epic.

## Cómo medirlo

En partida (`-game`, no PIE, que el subsistema sólo aplica el perfil en partida):

```
stat unit
stat streaming
stat gpu
memreport -full        (deja el informe en Saved/Profiling/MemReports)
alsasua.SetGraphicsProfile 1    (cambiar de perfil en caliente para comparar)
```

Lo que hay que mirar: en `stat streaming`, que `Required Pool` quepa en el pool
del perfil; en `memreport`, las 20 texturas y mallas más grandes, que son las
candidatas a bajar de resolución o a cocinarse con LOD bias.

## Lo que esto no cubre

- **Juego online.** La VRAM es de cada cliente; miles de jugadores a la vez
  piden servidor dedicado, replicación y relevancia de red, y el proyecto hoy
  no tiene capa de red. Un perfil de VRAM no lo resuelve.
- **El contenido.** El mayor ahorro real suele estar en los assets: texturas
  4K que nunca se ven de cerca, mallas sin Nanite ni LOD. Eso se decide con el
  `memreport` en la mano, no a ciegas.
