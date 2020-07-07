// relay.js

// spinner while awaiting for page load
$(document).ready(function () {
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function update_page() {
  update_contacts_list();
}

$('#contacts_refresh').on('click', function () {
  $('#awaiting').modal('show');
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_contacts_list();
});

// Relays

function esp_get_relay_list(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/relay',
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

function esp_get_relay_idx(ii, success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/relay/' + ii,
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

function esp_put_relay_idx(ii) {
  $.ajax({
    type: 'PUT',
    url: esp8266.url + '/api/relay/' + ii,
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ name: $('#relay_name').val(), logic: parseInt($('#relay_logic').val()), status_at_boot: parseInt($('#relay_default_status').val()) }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      $('#relayModal').modal('hide');
      update_contacts_list();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
      update_contacts_list();
    }
  });
}

function relay_logic(val) {
  switch (val) {
    case 0:
      return 'undefined';
    case 1:
      return 'pin LOW contact CLOSED';
    case 2:
      return 'pin HIGH contact CLOSED';
    default:
      return "";
  }
}

function relay_status_at_boot(val) {
  switch (val) {
    case 0:
      return 'undefined';
    case 1:
      return 'OPEN';
    case 2:
      return 'CLOSED';
    default:
      return '';
  }
}

function relay_reserved(val) {
  switch (val) {
    case 0:
      return '';
    case 1:
      return 'disabled';
    default:
      return '';
  }
}

function update_contacts_list() {
  esp_get_relay_list(function (data) {
    $("#contacts_table").empty();
    $("#contacts_table").append('<thead><tr><th scope="col">ID</th><th scope="col">Description</th><th scope="col">Logic</th><th scope="col">Default status</th><th scope="col">Actions</th></tr></thead><tbody>');
    for (var ii = 0; ii < data.relays.length; ii++) {
      $("#contacts_table").append('<tr><td>D' +
        (data.relays[ii].pin) +
        '</td><td>' +
        data.relays[ii].name +
        '</td><td>' +
        relay_logic(data.relays[ii].logic) +
        '</td><td>' +
        relay_status_at_boot(data.relays[ii].status_at_boot) +
        '</td><td><button class="btn btn-sm" id="cron_edit" onclick="modify_contact(' +
        data.relays[ii].pin +
        ')" ' +
        relay_reserved(data.relays[ii].reserved) +
        '><i class="fa fa-pencil-square-o"></i></button></td></tr>');
    }
    $("#contacts_table").append('</tbody>');
  });
}

function update_contact_idx(idx) {
  esp_get_relay_idx(idx, function (data) {
    $('#relay_name').val(data.name);
    $('#relay_logic').val(data.logic);
    $('#relay_default_status').val(data.status_at_boot);
  });
}

function modify_contact(idx) {
  $('#relayModalTitle').text('Contact D' + idx);
  update_contact_idx(idx);
  $('#relayModal').modal('show');
}

$('#relayModalReset').on('click', function () {
  update_contact_idx($('#relayModalTitle').text().slice(-1));
});

$('#relayModalSave').on('click', function () {
  esp_put_relay_idx($('#relayModalTitle').text().slice(-1));
});

$('#relay_name').on('change', function () {
  $('#relay_name').val($('#relay_name').val().slice(0, 31));
});