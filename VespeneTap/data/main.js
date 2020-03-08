const sampleGeyser = async () => {
  const response = await fetch('http://vespenegeyser32.local/house_status');
  const responseJson = await response.json(); //extract JSON from the http response

  console.log(responseJson['house_status']);

  sensors = responseJson['house_status'].split(";").sort();

  for(i = 0; i<sensors.length; i++){
    sensor_info = sensors[i].split(',');
    if(sensor_info.length !== 4){
      continue;
    }
    sensor_name = sensor_info[0];
    sensor_location = sensor_info[1]
    sensor_temperature = sensor_info[2];
    sensor_humidity = sensor_info[3];

    sensor_string = "<td>"+sensor_name + "</td><td>"+ sensor_location +"</td><td>"+sensor_temperature+"&deg;C</td><td> "+sensor_humidity+"%</td>";

    if($("#"+sensor_name).length === 0){
      $("table#sensorOutputs").append('<tr id="'+ sensor_name +'" class="sensor_entry">'+ sensor_string +"</tr>");
    }else{
      $("#"+sensor_name).html(sensor_string);
    }
  }

}

sampleGeyser();

var intervalID = setInterval(sampleGeyser, 10000);
