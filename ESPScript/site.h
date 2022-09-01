String html_1 = R"=====(
<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<title>ESP8266</title>
</head>
<body>
<style>
  .hidden {
    display:none;
  }
</style>
	<form name="ter" id="mars-once" method="POST">
	  <label>
	    SSID:<br/>
	    <input type="text" name="name" id="name">
	  </label><br/>
	  <label>
	    Password:<br/>
	    <input type="password" name="password">
	  </label><br/>
	  <label>
	    IP:<br/>
	    <input type="text" name="ip" id="name">
	  </label><br/>
	  <label>
	    Gateway:<br/>
	    <input type="text" name="gateway" id="name">
	  </label><br/>
	  <label>
	    Subnet:<br/>
	    <input type="text" name="subnet" id="name">
	  </label><br/>
	  <label>
	  <div id="loader" class="hidden">Подключение...</div>
  	<button type="submit">Подключиться</button>
</form>
<script>
	async function sendData() {
	    var formData = new FormData(document.forms.ter);

		  // отослать
		  var xhr = new XMLHttpRequest();
		  xhr.open("POST", "wifiSet");
		  xhr.send(formData);
			xhr.onload = function() {
				if (xhr.status != 200) {
					alert(`Подключение не удалось`); // Например, 404: Not Found
				} else { // если всё прошло гладко, выводим результат
					alert('Подключение успешно!')
					loader.classList.toggle('hidden')
				}
			}
	}

	async function handleFormSubmit(event) {
	  event.preventDefault()
	  const response = await sendData()
	  const loader = document.getElementById('loader')
		loader.classList.toggle('hidden')
	}
		
	const applicantForm = document.getElementById('mars-once')
	applicantForm.addEventListener('submit', handleFormSubmit)

	</script>
</body>
</html>
)=====";
