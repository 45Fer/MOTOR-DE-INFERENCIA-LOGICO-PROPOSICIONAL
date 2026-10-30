# Motor de inferencia lógico-proposicional

## Descripción

Este proyecto implementa un motor de inferencia lógico-proposicional en C++.
Su objetivo es recibir premisas y reglas expresadas en español, traducirlas a
fórmulas proposicionales, construir su árbol semántico y encontrar todas las
asignaciones de verdad que satisfacen simultáneamente el conjunto de entrada.

El sistema determina si el conjunto es:

- **Satisfactible:** existe al menos un modelo que hace verdaderas todas las
  premisas y reglas.
- **Insatisfactible:** no existe ninguna asignación que haga verdaderas todas
  las fórmulas al mismo tiempo.

## Pipeline del sistema

1. **Entrada de texto:** `main.cpp` recibe una regla por línea.
2. **Traducción del lenguaje natural:** `NaturalLanguageTranslator` normaliza
   la entrada y reemplaza conectores del español por operadores lógicos.
3. **Extracción de átomos:** `Formula::collectAtoms` obtiene el conjunto de
   proposiciones simples utilizadas.
4. **Análisis sintáctico:** `FormulaParser` construye un árbol de fórmulas
   respetando la precedencia de los operadores.
5. **Árbol semántico:** `SemanticTree` muestra la estructura de cada fórmula.
6. **Evaluación de modelos:** `ModelEvaluator` enumera las combinaciones de
   valores de verdad y conserva las que satisfacen todas las reglas.
7. **Reporte One Hot:** `ModelReport` muestra cada modelo como valores `V/F`
   y como un vector binario ordenado según la lista de átomos.

## Organización de carpetas

- `include/`: interfaces y tipos públicos (`.hpp`).
- `src/`: implementaciones (`.cpp`).
- `bin/`: objetos y ejecutable generado por `make`.
- `data/`: frases aprendidas que se conservan entre ejecuciones.
- `assets/`: recursos visuales y auditivos del proyecto.
- `docs/`: documentación y diagramas.

El archivo [diagrama-clases.puml](./diagrama-clases.puml) contiene el diagrama
UML de clases y sus relaciones.

## Lenguaje aceptado

El usuario puede escribir proposiciones como frases completas. El traductor
acepta español en minúsculas o mayúsculas, con o sin tildes, y variaciones
equivalentes de construcciones normativas:

- `no p`, `nunca p`: negación.
- `p y q`, `p además q`: conjunción.
- `p o q`, `p o bien q`: disyunción.
- `si p entonces q`: implicación.
- `p implica q`: implicación.
- `p si y solo si q`, `p equivale a q`: bicondicional.

Por ejemplo, estas entradas se interpretan como proposiciones completas:

```text
está lloviendo
el suelo está mojado
si está lloviendo entonces el suelo está mojado
está lloviendo y hace frío
no está lloviendo
```

Cada frase distinta se transforma internamente en un átomo identificador, pero
el reporte vuelve a mostrar el texto de la proposición para que el resultado
sea legible.

También se aceptan los operadores `!`, `&`, `|`, `->`, `<>` y paréntesis.
Cada línea de entrada representa una premisa o regla; todas deben cumplirse
simultáneamente para que un modelo sea reportado.

### Precedencia de operadores

De mayor a menor precedencia:

1. Negación: `!` o `~`.
2. Conjunción: `&` o `^`.
3. Disyunción: `|`.
4. Implicación: `->`.
5. Bicondicional: `<>`.

Los paréntesis pueden utilizarse para modificar el orden de evaluación.

## Ejemplo de ejecución

Entrada:

```text
si p entonces q
p
fin
```

Resultado esperado:

```text
SATISFACTIBLE
Átomos: p, q
Modelos: 1
{ p=V, q=V }  One Hot: [1, 1]
```

La interfaz gráfica solicita las frases en un editor multilinea. Cada frase
debe escribirse en su propia línea. El programa informa si el conjunto es
compatible, muestra los modelos válidos y presenta el árbol semántico.

## Interfaz gráfica nativa en C++

El proyecto incluye una interfaz de escritorio nativa Win32 implementada
completamente en C++. La ventana utiliza directamente `LogicEngine`, por lo que
no hay servidor, Python, HTML, JavaScript ni procesos externos.

Compila y abre la interfaz desde la raíz:

```text
make
make gui
```

En Windows también puedes ejecutar `bin/inferencia.exe` directamente. La
ventana permite escribir varias premisas, analizarlas, consultar el veredicto,
los modelos válidos, el vector One Hot y el árbol semántico.

## Memoria de frases aprendidas

Las proposiciones simples que el traductor reconoce se guardan en
`data/frases_aprendidas.txt`, una frase por línea. El archivo se carga al
iniciar el programa y se actualiza después de cada regla válida, por lo que
las frases conocidas permanecen disponibles en ejecuciones posteriores.
La memoria conserva el vocabulario del traductor; no agrega automáticamente
las frases anteriores como premisas de la sesión actual.

Ejemplo insatisfactible:

```text
p
no p
fin
```

Resultado:

```text
INSATISFACTIBLE
Modelos: 0
```

## Limitación actual

La evaluación utiliza enumeración exhaustiva de modelos. Por ello, el número
de combinaciones crece como `2^n`, donde `n` es el número de átomos distintos.
Esta estrategia es apropiada para conjuntos pequeños y permite mostrar todos
los modelos de manera transparente. Para conjuntos grandes, una futura mejora
sería incorporar un algoritmo basado en CNF, DPLL o un solucionador SAT.

## Compilación

Desde la raíz del proyecto:

```text
make
make gui
```

En Windows, si la consola muestra caracteres como `?` en lugar de vocales
acentuadas, configura la sesión de PowerShell en UTF-8 antes de ejecutar:

```powershell
chcp 65001
$OutputEncoding = [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()
.\bin\inferencia.exe
```

El programa también configura automáticamente la entrada y salida de la
consola de Windows como UTF-8.

El reporte usa `V` y `F` para los valores de verdad y determina si el conjunto
es satisfactible o insatisfactible. Además, cada modelo se muestra como un
vector One Hot binario en el mismo orden de la lista de átomos reportada.
