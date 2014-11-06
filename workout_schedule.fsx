open System

let rec exercises squats abs lifts =
    let perWeekFactor = 1.025 
    seq { yield (squats, abs, lifts) 
          yield! exercises (ceil <| perWeekFactor * squats) 
                           (ceil <| perWeekFactor * abs) 
                           (ceil <| perWeekFactor * lifts) }

let rec weeks (start:System.DateTime) = seq { yield start; yield! weeks <| start.AddDays(7.) }

let sched = Seq.zip (weeks <| System.DateTime.Parse "3-11-2014") (exercises 75. 75. 12.) |> Seq.map(fun(d,(a,b,c)) -> (d,a,b,c))

let schedEntryToString reps (d:System.DateTime, s, a, l) = 
    d.ToLongDateString() +
    (sprintf "->\n abs - %.0f, squats - %.0f, lifts - %.0f [x%.0f]\n" (s/reps) (a/reps) (l/reps) reps) 

let argv = System.Environment.GetCommandLineArgs() |> (fun args -> args.[3..]) // we need to skip first 3 args in lunix

let numOfReps, numOfWeeks = 
    match argv with
    | [|first; second|] -> (float first), (int second)
    | _ -> (3., 4)

sched |> Seq.skipWhile(fun (d, _, _, _) -> d < System.DateTime.Now)
      |> Seq.take numOfWeeks 
      |> Seq.map (schedEntryToString numOfReps) |> Seq.iter (printfn "%s")