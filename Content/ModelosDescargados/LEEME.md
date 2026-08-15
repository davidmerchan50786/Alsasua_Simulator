# ModelosDescargados

Props descargados de [Poly Haven](https://polyhaven.com), **licencia CC0**
(dominio público: no exige atribución, pero se deja constancia de los autores).

Rellenan los huecos que no cubría ni `AssetsImportados` ni Fab, comprobado
cruzando las palabras clave de `AlsasuaMallaFab` contra
`Datos/asset_manifest.json`.

| Modelo | Para | Autores |
|---|---|---|
| `fire_hydrant` | `boca_incendio` (8 piezas en street_furniture.json) | Poly Haven |
| `water_manhole_cover` | `tapa_alcantarilla` (6 piezas) | Poly Haven |
| `metal_trash_can` | `papelera` (97 piezas) | Poly Haven |
| `modular_street_seating` | `banco` (24 piezas) | Poly Haven |

glTF con texturas a 1k: 19,5 MB en total. Se eligió glTF sobre FBX porque pesa
la cuarta parte (el FBX de `fire_hydrant` solo son 22,6 MB frente a 5,3).

## Importar

    Tools/ImportarModelosDescargados.py

Deja los assets en `/Game/ModelosDescargados`, que es una de las raíces que
escanea `AlsasuaMallaFab`. A partir de ahí se prefieren a las mallas
procedurales de `/Game/Mobiliario`.

## Por qué no hay árboles aquí

Los árboles de Poly Haven son escaneos fotogramétricos: un solo pino son
672 MB en FBX 1k, y su FBX suelto 618 MB, por encima del límite de 100 MB por
fichero de GitHub. Y su catálogo no tiene roble, haya, abedul, fresno, chopo,
sauce, cerezo, tilo ni plátano, que es el 87 % del censo de `trees_unity.json`.
Para árboles: Fab, o las mallas procedurales de `/Game/Meshes/Arboles`.
