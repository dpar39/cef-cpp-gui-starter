import { Component } from '@angular/core';
import { setTheme } from 'ngx-bootstrap/utils';

@Component({
  selector: 'app-root',
  template: `
    <div class="container-fluid">
      <div class="row">
        <div class="col"><h1 class="text-center">This is a test</h1></div>
        <div class="col">
          <button
            type="button"
            class="btn btn-primary"
            tooltip="Vivamus sagittis lacus vel augue laoreet rutrum faucibus."
          >
            Simple demo
          </button>
        </div>
      </div>
    </div>
  `,
  styles: [''],
})
export class AppComponent {
  title = 'web';
  constructor() {
    setTheme('bs4'); // or 'bs3'
  }
}
