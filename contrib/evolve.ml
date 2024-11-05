type sensor = (int * int * int * bool)

type tank = (
  string
  * sensor
  * sensor
  * sensor
  * sensor
  * sensor
  * sensor
  * sensor
  * sensor
  * sensor
  * sensor
  * string list
)

let symbols = [
  "~";
  "!";
  "+";
  "-";
  "*";
  "/";
  "%";
  "&";
  "|";
  "^";
  "<<";
  ">>";
  ">";
  ">=";
  "<";
  "<=";
  "=";
  "<>";
  "abs";
  "dup";
  "pop";
  "exch";
  "if";
  "ifelse";
  "mset";
  "mget";
  "{";
  "}";
  "fire-ready?";
  "fire!";
  "set-speed!";
  "get-turret";
  "set-turret!";
  "sensor?";
  "set-led!";
  "random";
]

let make_nucleotide () =
  Random.int ((List.length symbols) + 360 + 360)
  

let rec make_dna = function
  | 0 ->
      []
  | n ->
      (make_nucleotide ()) :: (make_dna (n - 1))
  

let rec program_of_dna = function
  | [] ->
      []
  | i :: tl ->
      let s =
        try
          List.nth symbols i
        with
          | Failure _
          | Invalid_argument _ ->
              string_of_int (i - (List.length symbols) - 360)
      in
        s :: (program_of_dna tl)

let tank_of_dna d =
  let color_of_dna = function
    | r :: g :: b :: tl ->
        (tl, (Format.sprintf "%02x%02x%02x"
                ((abs r) mod 256)
                ((abs g) mod 256)
                ((abs b) mod 256)))
    | tl ->
        (tl, "cccccc")
  in
  let sensor_of_dna = function
    | a :: b :: c :: d :: tl ->
        (tl, (a, b, c, (d != 0)))
    | tl ->
        (tl, (0, 0, 0, false))
  in
  let d, color = color_of_dna d in
  let d, s0 = sensor_of_dna d in
  let d, s1 = sensor_of_dna d in
  let d, s2 = sensor_of_dna d in
  let d, s3 = sensor_of_dna d in
  let d, s4 = sensor_of_dna d in
  let d, s5 = sensor_of_dna d in
  let d, s6 = sensor_of_dna d in
  let d, s7 = sensor_of_dna d in
  let d, s8 = sensor_of_dna d in
  let d, s9 = sensor_of_dna d in
  let p = program_of_dna d in
    (color, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, p)


let print_tank t =
  let print_sensor (a, b, c, d) =
    Format.printf "Sensor: %d %d %d %d\n" a b c (if d then 1 else 0)
  in
    match t with
      | (color, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, p) ->
          Format.printf "Color: #%s\n" color;
          print_sensor s0;
          print_sensor s1;
          print_sensor s2;
          print_sensor s3;
          print_sensor s4;
          print_sensor s5;
          print_sensor s6;
          print_sensor s7;
          print_sensor s8;
          print_sensor s9;
          print_endline (String.concat " " p)

let write_tank i t =
  let fnprint fn str =
    let filename = Format.sprintf "tank.%d/%s" i fn in
    let chan = open_out filename in
      output_string chan str;
      close_out chan
  in
  let print_sensor n (a, b, c, d) =
    fnprint
      (Format.sprintf "sensor%d" n)
      (Format.sprintf "Sensor: %d %d %d %d\n" a b c (if d then 1 else 0))
  in
  match t with
    | (color, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, p) ->
        fnprint "color" color;
        print_sensor 0 s0;
        print_sensor 1 s1;
        print_sensor 2 s2;
        print_sensor 3 s3;
        print_sensor 4 s4;
        print_sensor 5 s5;
        print_sensor 6 s6;
        print_sensor 7 s7;
        print_sensor 8 s8;
        print_sensor 9 s9;
        fnprint "program" (String.concat " " p)
            
let _ = Random.self_init ()

let _ =
  let dnas =
    let t = Array.make 20 500 in
      Array.map make_dna t
  in
  let tanks = Array.map tank_of_dna dnas in
    Array.mapi write_tank tanks
