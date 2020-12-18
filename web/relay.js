// relay.js
$(document).ready(function () {
  esp_get_relay_list().then(function () {
    hide_spinner(500);
  })
});

$('#contacts_refresh').on('click', function () {
  show_spinner().then(function () {
    esp_get_relay_list().then(function () {
      hide_spinner(500);
    })
  })
});

// Relays

function esp_get_relay_list() {
  return esp_query({
    type: 'GET',
    url: '/api/relay',
    dataType: 'json',
    success: update_contacts_list,
    error: query_err
  });
}

function esp_get_relay_idx(ii) {
  return esp_query({
    type: 'GET',
    url: '/api/relay/' + ii,
    dataType: 'json',
    success: update_contact_idx,
    error: query_err
  });
}

function esp_put_relay_idx(ii) {
  return esp_query({
    type: 'PUT',
    url: '/api/relay/' + ii,
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ name: $('#relay_name').val(), logic: parseInt($('#relay_logic').val()), status_at_boot: parseInt($('#relay_default_status').val()) }),
    timeout: 8000,
    success: null,
    error: query_err
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

function update_contacts_list(data) {
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
}

function update_contact_idx(data) {
  $('#relay_name').val(data.name);
  $('#relay_logic').val(data.logic);
  $('#relay_default_status').val(data.status_at_boot);
}

function modify_contact(idx) {
  $('#relayModalTitle').text('Contact D' + idx);
  esp_get_relay_idx(idx).then(function () {
    $('#relayModal').modal('show');
  });
}

$('#relayModalReset').on('click', function () {
  esp_get_relay_idx($('#relayModalTitle').text().slice(-1));
});

$('#relayModalSave').on('click', function () {
  esp_put_relay_idx($('#relayModalTitle').text().slice(-1)).then(function () {
    $('#relayModal').modal('hide');
    show_spinner().then(function () {
      esp_get_relay_list().then(function () {
        hide_spinner(500);
      })
    })
  });
});

$('#relay_name').on('change', function () {
  $('#relay_name').val($('#relay_name').val().slice(0, 31));
});