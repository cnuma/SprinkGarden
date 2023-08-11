const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>cNuma GS (Garden Sprinkler)</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-0evHe/X+R7YkIZDRvuzKMRqM+OrBnVFBL6DOitfPri4tjfHxaWutUpFmBp4vmVor" crossorigin="anonymous">

    <script src="https://code.jquery.com/jquery-3.6.0.min.js" integrity"sha256-/xUj+3OJU5yExlq6GSYGSHk7tPXikynS7ogEvDej/m4=" crossorigin="anonymous"></script>
 
  </head>



  <style type="text/css">
    .button {
      background-color: #4CAF50; /* Green */
      border: none;
      color: white;
      padding: 15px 32px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
    }


    .row {
      width: 100%;
      display: flex;
      flex-direction: row;
      justify-content: center;
    }

    .blockLeft {
      display:flex;
      width: 100px;
      float: left;
      justify-content: center;
      align-content: center;
    }

    .blockRight {
      display:flex;
      width: 100px;
      float: right;
      justify-content: center;
      align-content: center;
    }

    .modal { 
    position: fixed; 
    top: 3%; 
    right: 3%; 
    left: 3%; 
    width: auto; 
    margin: 0; 
  }
  
  .modal-body { 
    height: 60%; 
  }

  .col-sm-2 {
    padding-left: 1px;
    padding-right: 1px;
    
    display: flex;
    justify-content: center;
    align-items: center;

  }

  .status0 {
    background-color: #A3A3A3;
  }

  .status1 {
    background-color: #98cd8d;
  }

  .status2 {
    background-image: linear-gradient(134deg, #98CD8D 25%, #F6F0CF 25%, #F6F0CF 50%, #98CD8D 50%, #98CD8D 75%, #F6F0CF 75%, #F6F0CF 100%);
    background-size: 55.61px 57.58px;
  }

  .vanne {
    font-size: 40px;
  }

  .vanne0 {
    background-color: #98cd8d;
  }

  .vanne1 {
    
  }

  



    
  </style>
  
<body style="background-color: #f9e79f " onload="refreshProg()">
  <center>
  <div class="container-fluid">
    <div class="row">
      <div class="col-6 col-sm-4" id=titre>
        <H2>cNuma GS
      </div>
      <div class="col-6 col-sm-4" id=heureESP>  
        hh:mm:ss</H2>
      </div>
    </div>

<!--
    <div class="row">
      <div class="col-6 col-sm-4">
        <button class="button" onclick="send(1)">Sprinkler ON</button>
      </div>
      <div class="col-6 col-sm-4">
        <button class="button" onclick="send(0)">Sprinkler OFF</button><BR>
      </div>
    </div>
--->
    
<!--  
    <div class="row"><h2>
      Vanne status: <span id="adc_val">0</span><br><br>
      Humidity : <span id="state">NA</span>
    </h2>
    </div>
-->
  
    <div class="row" >
      <div class="col-12 col-sm-12" id="progirri">
        Tableau des programmations ...
       </div>
    </div>

  </div>


  <div class="modal " tabindex="-1" id="modalProg">
  <div class="modal-dialog">
    <div class="modal-content">
      <div class="modal-header">
        <h5 class="modal-title" id="modalTitre">Programation Pxx</h5>
        <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
      </div>
      <div class="modal-body" id="corpsModale">
        
      </div>
      <div class="modal-footer">
        <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">Close</button>
        <button type="button" class="btn btn-primary saveChanges">Save changes</button>
      </div>
    </div>
  </div>
</div>





<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.2.0-beta1/dist/js/bootstrap.bundle.min.js" integrity="sha384-pprn3073KE6tl6bjs2QrFaJGz5/SUsLqktiwsUTF55Jfv3qYSDhgCecCxMW52nD2" crossorigin="anonymous"></script>

<script>
  
  function send(led_sts) 
  {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("state").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "led_set?state="+led_sts, true);
    xhttp.send();
  }


  
  setInterval(function() {
    getHeureESP();
  }, 1000); 


  // *** Mise à jour de la pendule de la page - valide que l'ESP est toujours en vie ;-) ***
  function getHeureESP() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
        //var retour = this.responseText;

        const retour = this.responseText.split("|");
        console.log("retour: " + retour);
        
        //document.getElementById("heureESP").innerHTML = this.responseText;
        document.getElementById("heureESP").innerHTML = retour[0];
        
        
        var els=document.getElementsByName("statusV0");
        console.log("els.length :" + els.length);
        
        for (var i=0; i<els.length; i++) {
          console.log("i: " + i);
          els[i].innerHTML = retour[1];
        }

        var els=document.getElementsByName("statusV1");
        console.log("els.length :" + els.length);
        
        for (var i=0; i<els.length; i++) {
          console.log("i: " + i);
          els[i].innerHTML = retour[2];
        }
        

      }
    };   
    
    xhttp.open("GET", "heureESP");
    xhttp.send();
  }

  // *** Fonction appelé au chargement de la page - onLoad - ***
  function refreshProg() {
    var xhttp = new XMLHttpRequest();

    xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
        document.getElementById("progirri").innerHTML =
        this.responseText;
      }
    };   
    
    xhttp.open("GET", "progirri");
    xhttp.send();
  }


   // **** Sur validation de la demande d'enregistrement des nouvelles valeurs du programme ****
   $(document).on('click', '.saveChanges', function() {
    
      console.log(".saveChanges appelé");

      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
          document.getElementById("progirri").innerHTML =
          this.responseText;
        }
      };   


      //programme
      //inputHH
      //inputMM
      //inputStatusProg
      //inputDelay
      
      xhttp.open("GET", "majProg?programme=" + $('#programme').val() 
                        + "&inputStatusProg=" + $('#inputStatusProg').val() 
                        + "&inputHH=" + $('#inputHH').val() 
                        + "&inputMM=" + $('#inputMM').val() 
                        + "&inputDelay=" + $('#inputDelay').val() 
                        + "&Lundi=" + $('#Lundi').is(":checked") 
                        + "&Mardi=" + $('#Mardi').is(":checked") 
                        + "&Mercredi=" + $('#Mercredi').is(":checked") 
                        + "&Jeudi=" + $('#Jeudi').is(":checked")
                        + "&Vendredi=" + $('#Vendredi').is(":checked")
                        + "&Samedi=" + $('#Samedi').is(":checked") 
                        + "&Dimanche=" + $('#Dimanche').is(":checked") 
                        + "&inputVanne=" + $('#inputVanne').val()
                        + "&inputHygrometrie=" +$('#inputHygrometrie').val()
                        + "&inputLabel=" +$('#inputLabel').val()  
                        
                        );
      xhttp.send();

      $('#modalProg').modal('hide');

      refreshProg();

      
   });

   

  $(document).on('click', '.ligneProg', function() {
      console.log('ligneProg clicked', event);
      console.log('data-prog:' + $(this).data("prog"));
    
      $("#modalTitre").html("<H2>Programme " + $(this).data("prog"));

      $("#corpsModale").html("Get Flash values !");
      var xhttp = new XMLHttpRequest();

      xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
          document.getElementById("corpsModale").innerHTML =
          this.responseText;
        }
      };   
      
      xhttp.open("GET", "progslot?programme=" + $(this).data("prog"));
      xhttp.send();
      

    
      $('#modalProg').modal('show');
  });



  
</script>



</center>
</body>
</html>
)=====";
