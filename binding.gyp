{
  "targets": [
    {
      "cflags": [ "-O3" ],
      "target_name": "weak-value-map",
      "conditions": [
        [ "OS=='linux'", { "cflags": [ "-fvisibility=hidden" ] } ]
      ],
      "sources": [ "weak-value-map.cc" ]
    }
  ]
}
