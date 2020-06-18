// debug.js

// spinner while awaiting for page load
$(document).ready(function () {
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function update_page() {
  counter = 1;
  update_gpio_list();
}


// Files

function esp_get_gpio_cfg(ii, success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/gpio/cfg/' + ii,
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

function esp_get_gpio_level(ii, success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/gpio/' + ii,
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(ii, data.gpio_level);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_gpio_level(idx, value) {
  var gpio_level_id = "#d" + idx + "_value";
  if (value == "low")
    $(gpio_level_id).prop('checked', true);
  else
    $(gpio_level_id).prop('checked', false);
}

function update_gpio(idx) {
  esp_get_gpio_cfg(idx, function (data) {
    var gpio_cfg_id = "#d" + idx + "_cfg";
    var gpio_level_id = "#d" + idx + "_value";
    switch (data.gpio_type) {
      case "unprovisioned":
        $(gpio_cfg_id).val(0);
        $(gpio_level_id).prop('disabled', true);
        break;
      case "input":
        $(gpio_cfg_id).val(1);
        esp_get_gpio_level(idx, update_gpio_level);
        $(gpio_level_id).prop('disabled', true);
        break;
      case "output":
        $(gpio_cfg_id).val(2);
        esp_get_gpio_level(idx, update_gpio_level);
        $(gpio_level_id).prop('disabled', false);
        break;
    }
  });
}

var counter = 1;

function update_gpio_list() {
  update_gpio(counter);
  counter++;
  if (counter > 8)
    return;
  setTimeout(function () {
    update_gpio_list();
  }, 100);
}

$('#gpio_refresh').on('click', function () {
  counter = 1;
  update_gpio_list();
});

function esp_set_gpio_level(ii, value) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/gpio/' + ii,
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ gpio_level: value }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      update_gpio(ii);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function gpio_set(idx) {
  var gpio_level_id = "#d" + idx + "_value";
  if ($(gpio_level_id).prop('checked'))
    esp_set_gpio_level(idx, "low");
  else
    esp_set_gpio_level(idx, "high");
}

function esp_set_gpio_cfg(ii, value) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/gpio/cfg/' + ii,
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ gpio_type: value }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      update_gpio(ii);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function gpio_cfg(idx) {
  var gpio_cfg_id = "#d" + idx + "_cfg";
  var value = $(gpio_cfg_id).val();
  switch (value) {
    case "0":
      esp_set_gpio_cfg(idx, "unprovisioned");
      var gpio_level_id = "#d" + idx + "_value";
      $(gpio_level_id).prop('checked', false);
      break;
    case "1":
      esp_set_gpio_cfg(idx, "input");
      break;
    case "2":
      esp_set_gpio_cfg(idx, "output");
      break;
  }
}