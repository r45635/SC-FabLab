<!DOCTYPE html>
<html>
<head>
  <!-- Include meta tag to ensure proper rendering and touch zooming -->
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- Include jQuery Mobile stylesheets -->
  <link rel="stylesheet" href="http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css">
  <!-- Include the jQuery library -->
  <script src="http://code.jquery.com/jquery-1.11.1.min.js"></script>
  <!--Load the AJAX API-->
  <script type="text/javascript" src="https://www.google.com/jsapi"></script>
  <!-- Include the jQuery Mobile library -->
  <script src="http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.js"></script>
  <meta charset="utf-8">
  <title>DIY - Fab Lab test!</title>
  <!-- Include Home Made CSS library -->
  <link rel="stylesheet" href="www-waterCounter.css">
  <link rel="icon" type="image/png" href="logo/jpg">	
  <!--  <script type="text/javascript" src="//ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js"></script> -->
  <script type="text/javascript">
      google.load('visualization', '1.0', {'packages':['corechart']});
//	  google.setOnLoadCallback(drawChart);
	  
var counter = 2;
myArray_Tick=["HOURLY","DAILY","RAW"];
var unit = " °C";
var tableName = "SENSOR_DHT";
var fieldName = "temperature";
var jsDataActualLoaded = false;

var jsonData_1, jsonData_2;

function myfailure() {
	alert('loading Error');
};

/*function lastDataBaseUpdate() {
$.ajax({
     type: "GET",
     url: '_getCCostDataDyn.php?f=lastDataBaseUpdate',
     success: function(data) {
          $('#lastDataBaseUpdate').html(" "+ data + ".");
     }

   });
};*/

function loadData() {

	waitloading();

$.when(
		$.get("_getData.php?t="+ tableName + "&f=" + fieldName + "&u=" + myArray_Tick[counter], function(jsonData_1_) {
			jsonData_1 = jsonData_1_;
		}),
		$.get("_getData.php?t="+ "SENSOR_DHT" + "&f=humidity&u=" + myArray_Tick[counter], function(jsonData_2_) {
			jsonData_2 = jsonData_2_;
		})
		).then(
		function() {
		jsDataActualLoaded = true;
		drawChartData();
});		  
		  
}

function drawChartData() {
	waitdisplay();
    var data_1 = new google.visualization.DataTable(jsonData_1);
    var chart_1 = new google.visualization.LineChart(document.getElementById('chart_div_1'));
	var options_1 = {
          title: tableName + ' ' + fieldName + ' / ' + myArray_Tick[counter] + ' (' + unit + ') ',
		  legend: 'none',
		  chartArea: {left:'30', top:'30', width:'95%'}
        };
    chart_1.draw(data_1, options_1);

	if (tableName == "SENSOR_DHT") {
    var data_2 = new google.visualization.DataTable(jsonData_2);
    var chart_2 = new google.visualization.LineChart(document.getElementById('chart_div_2'));
	var options_2 = {
          title: tableName + ' ' + 'Humidity / ' + myArray_Tick[counter] + ' (%) ',
		  legend: 'none',
		  chartArea: {left:'30', top:'30', width:'95%'}
        };
    chart_2.draw(data_2, options_2);
	}
	stoploading();
  }

function waitloading() {
$.mobile.loading( "show", {
                text: "loading...",
                textVisible: true,
                theme: 'b',
                textonly: false,
                html: ""
        });
};

function waitdisplay() {
$.mobile.loading( "show", {
                text: "Charts computing...",
                textVisible: true,
                theme: 'b',
                textonly: false,
                html: ""
        });
};

function stoploading() {
  $.mobile.loading( "hide" );
};

$(document).ready(function(){
  $("#ref_butn").click(function(){
 location.reload();
  });
  $("#ref_dsb").click(function(){
	tableName = "SENSOR_DSB";
	fieldName = "temperature";
	unit = "°C";
	$("#chart_div_2").hide();
	loadData();
  });  
  $("#ref_soil").click(function(){
	tableName = "SENSOR_SOIL";
	fieldName = "value";
	unit = " N/A ";
	$("#chart_div_2").hide();
	loadData();
  });
  $("#ref_dht").click(function(){
	tableName = "SENSOR_DHT";
	fieldName = "temperature";
	unit = "°C";	
	$("#chart_div_2").show();
	loadData();
  });  
  $("#ref_station").click(function(){
	tableName = "SENSOR_STATION";
	fieldName = "vcc";
	unit = "mV";	
	$("#chart_div_2").hide(); 
	loadData();
  });   
});

$(document).on("pageshow","#pageone",function(){
  if (!jsDataActualLoaded) {
	waitloading();
	loadData();
  }
});

$(function(){
  // Bind the swipeleftHandler callback function to the swipe event on div.box
 $("#chart_div_1").on( "swipeleft", swipeleftHandler );
 $("#chart_div_1").on( "swiperight", swiperightHandler );
 
  // Callback function references the event target and adds the 'swipeleft' class to it
  function swipeleftHandler( event ){
    counter=(counter-- % 3);
	loadData();
  };
  
  function swiperightHandler( event ){
    counter=(counter++ % 3);
	loadData();
  }  
});
</script>	
	
</head>
<body>

<div data-role="page" id="pageone">
 <div data-role="header">
    <a href="#home" class="ui-btn ui-mini ui-corner-all ui-shadow ui-icon-home ui-btn-icon-left ui-btn-icon-notext" data-transition="pop">Home</a>
	<a href="javascript:loadData();" id="refreshHomeButton" class="ui-btn ui-mini ui-corner-all ui-shadow ui-icon-refresh ui-btn-icon-left ui-btn-icon-notext">Refresh</a>
    <h1> SC Fab Lab Demo ! </h1>
    <!-- <a href="#" class="ui-btn ui-corner-all ui-shadow ui-icon-gear ui-btn-icon-right">Setup</a> -->
  </div>

  <div data-role="main" class="ui-content">
   <div class="outer_center">
    <div id="chart_div_1" class="inner-div" style="width:100%; align: center;" ></div><br>
	<div id="chart_div_2" class="inner-div" style="width:100%; align: center;" ></div>
   </div>
  </div>

  <div data-role="footer" style="text-align:center;">
	<button id="ref_dht" data-direction="reverse" data-role="button" data-theme="b" data-transition="pop">DHT</button>
	<button id="ref_soil" data-direction="reverse" data-role="button" data-theme="b" data-transition="pop">SOIL</button>
<!-- <button id="ref_butn" data-role="button" data-theme="b">Refresh</button> 
-->
	<button id="ref_dsb" data-direction="reverse" data-role="button" data-theme="b" data-transition="pop">DSB</button>
	<button id="ref_station" data-direction="reverse" data-role="button" data-theme="b" data-transition="pop">STATION</button>
  </div>
</div> 

</body>
</html>