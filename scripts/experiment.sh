#!/bin/sh

average() {
    MEAN="$(python -c "print(($(echo "$1" | sed 's/ / + /g')) / $2)")"
    STD="$(python -c "print((($(echo "$1" | sed 's/ /**2 + /g')**2) / $2 - $MEAN**2)**0.5)")"
    printf '%.2f\\pm%.2f' "$MEAN" "$STD"
}

DIR="$(dirname "$0")/.."

GEN="$DIR/src/generator/generate.py"
MAN="$DIR/src/manager/bin/manager"

INT="$DIR/config/$1.json"
RATE="$2"
TICKS="$3"
N="$4"

for i in $(seq "$N"); do
    python "$GEN" -o "$1-$i.json" -r "$RATE" -t "$TICKS" "$INT"
done

tabs 20
for i in 1 2 3 4; do
    RTS=''
    TLS=''
    TDS=''
    for j in $(seq "$N"); do
        RES="$(./$MAN "$1-$j.json" /dev/null "$i" | cut -d : -f 2 | tr -d ' ')"
        ALGO="$(echo "$RES" | sed -n '1p')"
        M="$(echo "$RES" | sed -n '2p')"
        RTS="$RTS $(echo "$RES" | sed -n '3p')"
        TLS="$TLS $(echo "$RES" | sed -n '5p')"
        TDS="$TDS $(echo "$RES" | sed -n '6p')"
    done
    RT="$(average "$(echo "$RTS" | cut -c 2-)" "$N")"
    TL="$(average "$(echo "$TLS" | cut -c 2-)" "$N")"
    TD="$(average "$(echo "$TDS" | cut -c 2-)" "$N")"
    echo -e "$ALGO & \$$TL\$ & \$$TD\$ & \$$RT\$ \\\\\\"
done
echo "M = $M"

