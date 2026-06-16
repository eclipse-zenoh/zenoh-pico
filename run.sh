nb=${1:-5}
duration=${2:-60}
connect_timeout=${3:--1}

echo "" > log
for i in $(seq -f "%03g" 1 1 $nb 2>/dev/null)
do
  connect=""
  for j in $(seq -f "%03g" 1 1 $((10#$i - 1)) 2>/dev/null)
  do
    connect="$connect -e tcp/127.0.0.1:8$j -e tcp/127.0.0.1:9$j"
  done
  stdbuf -o0 ./build/examples/z_pub -m peer -l tcp/127.0.0.1:8$i -t $connect_timeout $connect -k demo/example/$i | while read line; do echo "[pub $i][$(date +%s.%N)] $line"; done >> log &
  stdbuf -o0 ./build/examples/z_sub -m peer -l tcp/127.0.0.1:9$i -t $connect_timeout -e tcp/127.0.0.1:8$i $connect | while read line; do echo "[sub $i][$(date +%s.%N)] $line"; done >> log &
done

sleep $duration

killall z_sub z_pub

start=""
stop=""
failure=""

result=$(awk -v nb="$nb" '
  /^\[/ && start == "" {
    split($0, ts, "[")
    split(ts[3], sec, ".")
    start = sec[1]
  }

  /^\[sub [0-9][0-9][0-9]\]\[/ && /Received/ && /demo\/example\// {
    split($2, sub_parts, "]")
    subscriber = sub_parts[1] + 0
    split($0, key_parts, "demo/example/")
    split(key_parts[2], pub_parts, ":")
    publisher = pub_parts[1] + 0
    key = subscriber "," publisher
    if (!(key in status)) {
      status[key] = 1
      count++
    }
    split($0, ts, "[")
    split(ts[3], sec, ".")
    stop = sec[1]
    if (count == nb * nb) {
      print "OK", start, stop
      exit
    }
  }

  END {
    for (subscriber = 1; subscriber <= nb; subscriber++) {
      for (publisher = 1; publisher <= nb; publisher++) {
        if (!((subscriber "," publisher) in status)) {
          print "KO", subscriber, publisher
          exit
        }
      }
    }
    print "OK", start, stop
  }
' log)

read -r result_status first second <<< "$result"
if [[ $result_status == "OK" ]]
then
  start=$first
  stop=$second
else
  failure="Sub $first didn't receive from Pub $second"
fi

if [[ $stop != "" ]]
then
  echo OK $(($stop - $start)) seconds
else
  echo KO $failure
fi
