import { IndiTrans } from './IndiTrans';
import { TranslitOptions } from './TranslitOptions';

export { IndiTrans, TranslitOptions };

IndiTrans.instance.then((i) => {
  console.log(i.transliterate('கப் ப', 'tamil', 'iast', TranslitOptions.None));
});
