// Little helper to generate the ttl for the control ports more easily

object ControlPortGenerator extends App {
  val elements = for {
    gen <- 0 to 15
    offset = 1 + gen * 8
  } yield
    s"""
       |  # generator $gen
       |  [
       |    a lv2:InputPort, lv2:ControlPort ;
       |    lv2:index ${offset + 0} ;
       |    lv2:symbol "switch_$gen" ;
       |    lv2:name "Enabled" ;
       |    lv2:default 0 ;
       |    lv2:portProperty lv2:toggled ;
       |  ], [
       |    a lv2:InputPort, lv2:ControlPort ;
       |    lv2:index ${offset + 1} ;
       |    lv2:symbol "beats_$gen" ;
       |    lv2:name "Number of beats" ;
       |    lv2:minimum 2 ;
       |    lv2:maximum 64 ;
       |    lv2:default 8 ;
       |    lv2:portProperty lv2:integer ;
       |  ], [
       |    a lv2:InputPort, lv2:ControlPort ;
       |    lv2:index ${offset + 2} ;
       |    lv2:symbol "onsets_$gen" ;
       |    lv2:name "Number of onsets" ;
       |    lv2:minimum 0 ;
       |    lv2:maximum 64 ;
       |    lv2:default 5 ;
       |    lv2:portProperty lv2:integer ;
       |  ], [
       |    a lv2:InputPort, lv2:ControlPort ;
       |    lv2:index ${offset + 3} ;
       |    lv2:symbol "rotation_$gen" ;
       |    lv2:name "Number of places the pattern is rotated" ;
       |    lv2:minimum -32 ;
       |    lv2:maximum 31 ;
       |    lv2:default 0 ;
       |    lv2:portProperty lv2:integer ;
       |  ], [
       |    a lv2:InputPort, lv2:ControlPort ;
       |    lv2:index ${offset + 4} ;
       |    lv2:symbol "bars_$gen" ;
       |    lv2:name "Pattern size (in bars)" ;
       |    lv2:minimum 1 ;
       |    lv2:maximum 8 ;
       |    lv2:default 1 ;
       |    lv2:portProperty lv2:integer ;
       |  ], [
       |    a lv2:InputPort, lv2:ControlPort ;
       |    lv2:index ${offset + 5} ;
       |    lv2:symbol "channel_$gen" ;
       |    lv2:name "MIDI channel to use" ;
       |    lv2:minimum 1 ;
       |    lv2:maximum 16 ;
       |    lv2:default 10 ;
       |    lv2:portProperty lv2:integer ;
       |  ], [
       |    a lv2:InputPort, lv2:ControlPort ;
       |    lv2:index ${offset + 6} ;
       |    lv2:symbol "note_$gen" ;
       |    lv2:name "Note to play" ;
       |    lv2:minimum 0 ;
       |    lv2:maximum 127 ;
       |    lv2:default 48 ;
       |    lv2:portProperty lv2:integer ;
       |  ], [
       |    a lv2:InputPort, lv2:ControlPort ;
       |    lv2:index ${offset + 7} ;
       |    lv2:symbol "velocity_$gen" ;
       |    lv2:name "Note velocity" ;
       |    lv2:minimum 0 ;
       |    lv2:maximum 127 ;
       |    lv2:default 64 ;
       |    lv2:portProperty lv2:integer ;
       |  ],
       |
       |""".stripMargin

  println(elements.mkString)
}
