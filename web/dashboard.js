// dashoard.js

// spinner while awaiting for page load
$(document).ready(function () {
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function update_page() {
  update_device_info();
}

// device info

function esp_get_info(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/info',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_device_info() {
  esp_get_info(function (data) {
    $("#app_name").val(data.app_name);
    $("#app_version").val(data.app_version);
    $("#dev_name").val(data.device_name);
    $("#espbot_version").val(data.espbot_version);
    $("#library_version").val(data.library_version);
    $("#chip_id").val(data.chip_id);
    $("#sdk_version").val(data.sdk_version);
    $("#boot_version").val(data.boot_version);
  });
}

$('#info_edit').on('click', function () {
  if ($('#info_buttons').hasClass("d-none")) {
    $('#info_buttons').removeClass("d-none");
    $('#dev_name').removeClass("border-0");
  }
  else {
    $('#info_buttons').addClass("d-none");
    $('#dev_name').addClass("border-0");
  }
});

$('#info_reset').on('click', function () {
  update_device_info();
});

$('#info_save').on('click', function (e) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/deviceName',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ device_name: $('#dev_name').val() }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      alert("Device name saved.");
      update_device_info();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  })
});